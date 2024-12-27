#include "core/config/config.h"
#include "function/type/light.h"
#include <glm/glm.hpp>

struct FireLights;

class FireLightsUpdater {
public:
    void init(const FieldsConfiguration& config);
    void updateFireLightData(cudaSurfaceObject_t fire_image, cudaStream_t streamToRun, Lights& lights);
    void destroy();

    glm::ivec3 sample_dim;
    glm::ivec3 sample_avg_region;
    glm::ivec3 sample_kernel_size;
    glm::ivec3 field_dim;

private:
    void loadFireColorTexture(const std::string& path);

    cudaArray_t fire_color_array;
    cudaTextureObject_t fire_color_texture;

    glm::vec3* d_out_intensities;
    std::vector<glm::vec3> out_intensities;
};
