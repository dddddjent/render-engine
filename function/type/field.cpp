#include "field.h"
#include "core/math/math.h"
#include "core/tool/logger.h"
#include "core/tool/npy.hpp"
#include "core/vulkan/vulkan_util.h"
#include "function/global_context.h"
#include "function/resource_manager/resource_manager.h"
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <core/tool/vtk.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef _WIN64
#include <dxgi1_2.h>
#endif

using cudaArray_t         = unsigned long long*;
using cudaTextureObject_t = unsigned long long*;
using cudaSurfaceObject_t = unsigned long long*;
using cudaStream_t        = unsigned long long*;
#include "function/tool/fire_light_updater.h"

using namespace Vk;

Fields::Fields()                    = default;
Fields::~Fields()                   = default;
Fields::Fields(Fields&& f) noexcept = default;
Fields& Fields::operator=(Fields&& f) noexcept
{
    if (this != &f) {
        this->fields      = std::move(f.fields);
        this->step        = std::move(f.step);
        this->paramBuffer = std::move(f.paramBuffer);

        this->has_temperature          = std::move(f.has_temperature);
        this->lights_dim               = std::move(f.lights_dim);
        this->lights                   = std::move(f.lights);
        this->lights_updater           = std::move(f.lights_updater);
        this->self_illumination_lights = std::move(f.self_illumination_lights);
        this->fire_color_img           = std::move(f.fire_color_img);
    }
    return *this;
};

void Field::destroy()
{
    Buffer::Delete(g_ctx.vk, attr_buf);
    Image::Delete(g_ctx.vk, field_img);
}

void Field::init(const FieldConfiguration& cfg)
{
    name = cfg.name;

    attr_buf = Buffer::New(
        g_ctx.vk,
        sizeof(FieldData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    attr_buf.Update(g_ctx.vk, &data, sizeof(FieldData));
    g_ctx.dm.registerResource(attr_buf, DescriptorType::Uniform);

    initFieldImage(cfg);
    g_ctx.dm.registerResource(field_img, DescriptorType::CombinedImageSampler);
}

std::vector<float> Field::loadFieldData(const FieldConfiguration& cfg)
{
    std::string extension_name = std::filesystem::path(cfg.path).extension().string();

    std::vector<float> data;
    if (extension_name == ".npy") {
        npy::npy_data d         = npy::read_npy<float>(cfg.path);
        data                    = d.data;
        const auto& image_shape = d.shape;
        assert(image_shape[0] == cfg.dimension[0]);
        assert(image_shape[1] == cfg.dimension[1]);
        assert(image_shape[2] == cfg.dimension[2]);
    } else if (extension_name == ".vti") {
        data = readVti<float>(cfg.path, cfg.name, nullptr);
    } else {
        ERROR_ALL("Unknown file extension \"" + extension_name + "\"");
        throw std::runtime_error("Unknown file extension \"" + extension_name + "\"");
    }

    return data;
}

void Field::initFieldImage(const FieldConfiguration& cfg)
{
    const auto image_data = loadFieldData(cfg);

    auto extent = VkExtent3D {
        static_cast<uint32_t>(cfg.dimension[0]),
        static_cast<uint32_t>(cfg.dimension[1]),
        static_cast<uint32_t>(cfg.dimension[2]),
    };
    field_img = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        true,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_3D,
        VK_IMAGE_VIEW_TYPE_3D);
    field_img.Update(g_ctx.vk, image_data.data());
    field_img.AddDefaultSampler(g_ctx.vk);
    field_img.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Field::updateFieldImage(const std::vector<float>& data)
{
    field_img.Update(g_ctx.vk, data.data());
}

void SelfIlluminationLights::destroy()
{
    Buffer::Delete(g_ctx.vk, buffer);
}

void SelfIlluminationLights::init(FieldsConfiguration& cfg)
{
    VkDeviceSize bufferSize = sizeof(glm::vec4) * positions.size();
    buffer                  = Buffer::New(
        g_ctx.vk,
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    buffer.Update(g_ctx.vk, positions.data(), bufferSize);
    g_ctx.dm.registerResource(buffer, DescriptorType::Storage);
}

void Fields::destroy()
{
    for (auto& field : fields) {
        field.destroy();
    }
    Buffer::Delete(g_ctx.vk, paramBuffer);

    if (has_temperature) {
        lights.destroy();
        lights_updater->destroy();
        self_illumination_lights.destroy();
        Image::Delete(g_ctx.vk, fire_color_img);
    }
}

void Fields::initFireLights(FieldsConfiguration& cfg)
{
    lights.name = "fire_lights";

    FireConfiguration fire_cfg = cfg.fire_configuration;
    lights_dim.x               = fire_cfg.light_sample_dim[0];
    lights_dim.y               = fire_cfg.light_sample_dim[1];
    lights_dim.z               = fire_cfg.light_sample_dim[2];
    int total_num              = lights_dim.x * lights_dim.y * lights_dim.z;

    int temperature_field_idx = -1;
    for (int i = 0; i < cfg.arr.size(); i++) {
        if (cfg.arr[i].data_type == "temperature") {
            temperature_field_idx = i;
            break;
        }
    }
    assert(temperature_field_idx != -1);
    const auto& temperature_field = cfg.arr[temperature_field_idx];

    float step_x        = temperature_field.size[0] / lights_dim.x;
    float step_y        = temperature_field.size[1] / lights_dim.y;
    float step_z        = temperature_field.size[2] / lights_dim.z;
    glm::vec3 start_pos = arrayToVec3(temperature_field.start_pos) + glm::vec3(step_x, step_y, step_z) / 2.0f;
    for (int z = 0; z < lights_dim.z; z++) {
        for (int y = 0; y < lights_dim.y; y++) {
            for (int x = 0; x < lights_dim.x; x++) {
                LightData light;
                light.posOrDir
                    = start_pos + glm::vec3(x * step_x, y * step_y, z * step_z);
                light.intensity
                    = glm::vec3(0.0f, 0.0f, 0.0f);
                lights.data.emplace_back(light);
            }
        }
    }
    lights.buffer = Buffer::New(
        g_ctx.vk,
        total_num * sizeof(LightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    lights.update(lights.data.data(), 0, total_num);
    g_ctx.dm.registerResource(lights.buffer, DescriptorType::Storage);
}

void Fields::initFireColorImage(FieldsConfiguration& cfg)
{
    const std::string& fire_colors_path = cfg.fire_configuration.at("fire_colors_path");
    npy::npy_data d                     = npy::read_npy<float>(fire_colors_path);
    const auto& image_data              = d.data;
    const auto& image_shape             = d.shape;
    VkDeviceSize image_size             = image_shape[0] * image_shape[1] * sizeof(float); // RGBA * sizeof(float)

    const auto extent = VkExtent3D {
        static_cast<uint32_t>(image_shape[0]),
        1,
        1,
    };
    fire_color_img = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32G32B32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_LINEAR);
    fire_color_img.Update(g_ctx.vk, image_data.data());
    fire_color_img.AddDefaultSampler(g_ctx.vk);
    fire_color_img.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(fire_color_img, DescriptorType::CombinedImageSampler);
}

Fields Fields::fromConfiguration(FieldsConfiguration& cfg)
{
    Fields fields;
    fields.step            = cfg.step;
    fields.has_temperature = false;

    assert(cfg.arr.size() <= MAX_FIELDS);
    int temp_field_cnt = 0;
    for (const auto& field_config : cfg.arr) {
        Field field;

        field.name = field_config.name;

        glm::vec3 start_pos     = arrayToVec3(field_config.start_pos);
        glm::vec3 size          = arrayToVec3(field_config.size);
        field.data.to_local_uvw = Fields::toLocaluvw(g_ctx.rm->camera, start_pos, size);
        field.data.scatter      = arrayToVec3(field_config.scatter);
        field.data.absorption   = arrayToVec3(field_config.absorption);
        field.data.aabb         = AABB { .bmin = start_pos, .bmax = start_pos + size };
        if (field_config.data_type == "concentration") {
            field.data.type = FieldDataType::CONCENTRATION;
        } else if (field_config.data_type == "temperature") {
            assert(temp_field_cnt == 0 && "Only one temperature field is supported");
            field.data.type        = FieldDataType::TEMPERATURE;
            fields.has_temperature = true;
        }

        field.init(field_config);
        fields.fields.emplace_back(field);
    }

    if (fields.has_temperature) {
        FireConfiguration fire_cfg = cfg.fire_configuration;
        fields.initFireLights(cfg);
        fields.initFireColorImage(cfg);

        for (const auto& light_config_pos : fire_cfg.self_illumination_lights)
            fields.self_illumination_lights.positions.emplace_back(
                glm::vec4(arrayToVec3(light_config_pos), 0.f));
        fields.self_illumination_lights.init(cfg);

        fields.lights_updater = std::make_unique<FireLightsUpdater>();
        fields.lights_updater->init(cfg);
    }

    for (int i = 0; i < fields.fields.size(); i++) {
        fields.param.attr[i * 4]
            = g_ctx.dm.getResourceHandle(fields.fields[i].attr_buf.id);
        fields.param.img[i * 4]
            = g_ctx.dm.getResourceHandle(fields.fields[i].field_img.id);
    }
    fields.paramBuffer = Buffer::New(
        g_ctx.vk,
        sizeof(Param),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    fields.paramBuffer.Update(g_ctx.vk, &fields.param, sizeof(Param));
    g_ctx.dm.registerParameter(fields.paramBuffer);

    return fields;
}

glm::mat4x4 Fields::toLocaluvw(const Camera& camera, const glm::vec3& start_pos, const glm::vec3& size)
{
    glm::mat4x4 mat(1.0f);
    mat = glm::scale(mat, glm::vec3(1 / size.x, 1 / size.y, 1 / size.z));
    mat = glm::translate(mat, -start_pos);
    return mat;
}

#ifdef _WIN64
HANDLE Fields::getVkFieldMemHandle(int index)
{
    HANDLE handle;
    VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
    vkMemoryGetWin32HandleInfoKHR.sType                         = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    vkMemoryGetWin32HandleInfoKHR.memory                        = fields[index].field_img.memory;
    vkMemoryGetWin32HandleInfoKHR.handleType                    = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    fpGetMemoryWin32Handle(g_ctx.vk.device, &vkMemoryGetWin32HandleInfoKHR, &handle);
    return handle;
}

HANDLE Fields::getVkFieldMemHandle(const std::string& field_name)
{
    HANDLE handle;
    VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
    vkMemoryGetWin32HandleInfoKHR.sType                         = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    for (const auto& field : fields) {
        if (field.name == field_name) {
            vkMemoryGetWin32HandleInfoKHR.memory = field.field_img.memory;
            break;
        }
    }
    vkMemoryGetWin32HandleInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    fpGetMemoryWin32Handle(g_ctx.vk.device, &vkMemoryGetWin32HandleInfoKHR, &handle);
    return handle;
}
#else
int Fields::getVkFieldMemHandle(int index)
{
    int fd;
    VkMemoryGetFdInfoKHR vkMemoryGetFdInfoKHR = {};
    vkMemoryGetFdInfoKHR.sType                = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    vkMemoryGetFdInfoKHR.memory               = fields[index].field_img.memory;
    vkMemoryGetFdInfoKHR.handleType           = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    fpGetMemoryFdKHR(g_ctx.vk.device, &vkMemoryGetFdInfoKHR, &fd);
    return fd;
}

int Fields::getVkFieldMemHandle(const std::string& field_name)
{
    int fd;
    VkMemoryGetFdInfoKHR vkMemoryGetFdInfoKHR = {};
    vkMemoryGetFdInfoKHR.sType                = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    for (const auto& field : fields) {
        if (field.name == field_name) {
            vkMemoryGetFdInfoKHR.memory = field.field_img.memory;
            break;
        }
    }
    vkMemoryGetFdInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    fpGetMemoryFdKHR(g_ctx.vk.device, &vkMemoryGetFdInfoKHR, &fd);
    return fd;
}
#endif
