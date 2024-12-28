#include "core/math/math.h"
#include "core/tool/npy.hpp"
#include "fire_light_updater.h"
#include "function/type/light.h"
#include <cuda.h>
#include <cuda_runtime.h>

#define get_bid() (blockIdx.z * gridDim.y * gridDim.x + blockIdx.y * gridDim.x + blockIdx.x)
#define get_tid() (get_bid() * (blockDim.x * blockDim.y * blockDim.z) + threadIdx.z * blockDim.x * blockDim.y + threadIdx.y * blockDim.x + threadIdx.x)

__device__ float light_sample_gain = 0.1f;

void FireLightsUpdater::init(const FieldsConfiguration& config)
{
    FireConfiguration fire_cfg = config.fire_configuration;
    sample_dim                 = glm::ivec3(
        fire_cfg.light_sample_dim[0],
        fire_cfg.light_sample_dim[1],
        fire_cfg.light_sample_dim[2]);
    sample_avg_region = glm::ivec3(
        fire_cfg.light_sample_avg_region[0],
        fire_cfg.light_sample_avg_region[1],
        fire_cfg.light_sample_avg_region[2]);
    for (const auto& field : config.arr) {
        if (field.name == "fire_field") {
            field_dim = arrayToVec3(field.dimension);
        }
    }
    // assert(field_dim.x % sample_dim.x == 0 && field_dim.y % sample_dim.y == 0 && field_dim.z % sample_dim.z == 0);
    sample_kernel_size = glm::ivec3(
        ceil(field_dim.x / (float)sample_dim.x),
        ceil(field_dim.y / (float)sample_dim.y),
        ceil(field_dim.z / (float)sample_dim.z));

    loadFireColorTexture(fire_cfg.fire_colors_path);

    cudaMalloc(&d_out_intensities, sample_dim.x * sample_dim.y * sample_dim.z * sizeof(glm::vec3));
    out_intensities.resize(sample_dim.x * sample_dim.y * sample_dim.z);

    auto light_sample_gain_host = fire_cfg.light_sample_gain;
    cudaMemcpyToSymbol(light_sample_gain, &light_sample_gain_host, sizeof(float));
}

void FireLightsUpdater::loadFireColorTexture(const std::string& path)
{
    npy::npy_data d         = npy::read_npy<float>(path);
    const auto& image_data  = d.data;
    const auto& image_shape = d.shape;
    assert(image_shape.size() == 2);
    assert(image_shape[1] == 3);
    std::vector<float4> image_data4(image_data.size() / 3);
    for (int i = 0; i < image_data.size() / 3; i++) {
        image_data4[i].x = image_data[i * 3 + 0];
        image_data4[i].y = image_data[i * 3 + 1];
        image_data4[i].z = image_data[i * 3 + 2];
        image_data4[i].w = 0.0f;
    }

    cudaChannelFormatDesc formatDesc = cudaCreateChannelDesc<float4>();
    cudaMallocArray(&fire_color_array, &formatDesc, image_shape[0], 1);
    cudaMemcpy2DToArray(fire_color_array, 0, 0, image_data4.data(),
                        image_shape[0] * sizeof(float4),
                        image_shape[0] * sizeof(float4),
                        1, cudaMemcpyHostToDevice);

    cudaResourceDesc res_desc;
    memset(&res_desc, 0, sizeof(res_desc));
    res_desc.resType         = cudaResourceTypeArray;
    res_desc.res.array.array = fire_color_array;
    cudaTextureDesc tex_desc;
    {
        memset(&tex_desc, 0, sizeof(tex_desc));
        tex_desc.addressMode[0]   = cudaAddressModeClamp;
        tex_desc.addressMode[1]   = cudaAddressModeClamp;
        tex_desc.filterMode       = cudaFilterModeLinear;
        tex_desc.readMode         = cudaReadModeElementType;
        tex_desc.normalizedCoords = 1;
    }
    cudaCreateTextureObject(&fire_color_texture, &res_desc, &tex_desc, NULL);
}

__global__ void updateKernel(
    cudaSurfaceObject_t fire_image,
    glm::ivec3 fire_image_dim,
    cudaTextureObject_t fire_color_texture,
    glm::ivec3 sample_dim,
    glm::ivec3 sample_avg_region,
    glm::ivec3 sample_kernel_size,
    glm::vec3* out_intensities)
{
    auto tid = get_tid();

    auto& out = out_intensities[tid];
    int z     = tid / (sample_dim.x * sample_dim.y);
    int y     = (tid % (sample_dim.x * sample_dim.y)) / sample_dim.x;
    int x     = tid % sample_dim.x;
    if (x >= sample_dim.x || y >= sample_dim.y || z >= sample_dim.z) {
        return;
    }
    int3 image_xyz = make_int3(
        x * sample_kernel_size.x + sample_kernel_size.x / 2,
        y * sample_kernel_size.y + sample_kernel_size.y / 2,
        z * sample_kernel_size.z + sample_kernel_size.z / 2);

    glm::dvec3 color = glm::dvec3(0.0f);
    for (int i = image_xyz.z - sample_avg_region.z; i <= image_xyz.z + sample_avg_region.z; i++) {
        for (int j = image_xyz.y - sample_avg_region.y; j <= image_xyz.y + sample_avg_region.y; j++) {
            for (int k = image_xyz.x - sample_avg_region.x; k <= image_xyz.x + sample_avg_region.x; k++) {
                auto temperature = surf3Dread<float>(
                    fire_image,
                    k * sizeof(float),
                    j,
                    i,
                    cudaBoundaryModeClamp);
                float4 temp_color = tex1D<float4>(fire_color_texture, temperature);
                color += glm::vec3(temp_color.x, temp_color.y, temp_color.z);
            }
        }
    }
    color /= (2 * sample_avg_region.x + 1)
        * (2 * sample_avg_region.y + 1)
        * (2 * sample_avg_region.z + 1) / light_sample_gain;
    out = color;
}

void FireLightsUpdater::updateFireLightData(cudaSurfaceObject_t fire_image, cudaStream_t streamToRun, Lights& lights)
{
    dim3 thread_dim(
        std::min(sample_dim.x, 4),
        std::min(sample_dim.y, 4),
        std::min(sample_dim.z, 4));
    dim3 block_dim(
        (sample_dim.x + 3) / 4,
        (sample_dim.y + 3) / 4,
        (sample_dim.z + 3) / 4);
    updateKernel<<<block_dim, thread_dim, 0, streamToRun>>>(
        fire_image,
        field_dim,
        fire_color_texture,
        sample_dim,
        sample_avg_region,
        sample_kernel_size,
        d_out_intensities);
    cudaMemcpy(
        out_intensities.data(),
        d_out_intensities,
        sample_dim.x * sample_dim.y * sample_dim.z * sizeof(float3),
        cudaMemcpyDeviceToHost);

    for (int i = 0; i < sample_dim.x * sample_dim.y * sample_dim.z; i++) {
        lights.data[i].intensity = out_intensities[i];
    }
    lights.update(lights.data.data(), 0, lights.data.size());
}

void FireLightsUpdater::destroy()
{
    cudaFreeArray(fire_color_array);
    cudaDestroyTextureObject(fire_color_texture);
    cudaFree(&d_out_intensities);
}
