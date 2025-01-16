#pragma once

#include "aabb.h"
#include "camera.h"
#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/type/image.h"
#include "light.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#ifdef _WIN64
#include <Windows.h>
#endif

enum class FieldDataType : uint32_t {
    CONCENTRATION = 0,
    TEMPERATURE   = 1,
};

#ifdef MAX_FIELD_EXT
inline constexpr uint32_t MAX_FIELDS = MAX_FIELD_EXT;
#else
inline constexpr uint32_t MAX_FIELDS = 2;
#endif

struct FieldData {
    glm::mat4x4 to_local_uvw;
    glm::vec3 scatter;
    FieldDataType type;
    glm::vec3 absorption;
    float padding0;
    AABB aabb;
};

struct Field {
    std::string name;

    FieldData data;
    Vk::Buffer attr_buf;

    Vk::Image field_img;

    void destroy();
    void init(const FieldConfiguration& cfg);
    void updateFieldImage(const std::vector<float>& data);

private:
    void initFieldImage(const FieldConfiguration& cfg);
    std::vector<float> loadFieldData(const FieldConfiguration& cfg);
};

class FireLightsUpdater;

struct SelfIlluminationLights {
    std::string name = "self_illumination_lights";

    std::vector<std::string> ids;

    std::vector<glm::vec4> positions;
    Vk::Buffer buffer;

    void destroy();
    void init(FieldsConfiguration& cfg);
};

struct Fields {
    struct Param {
        Vk::DescriptorHandle attr[MAX_FIELDS * 4]; // * 4 for alignment
        Vk::DescriptorHandle img[MAX_FIELDS * 4];
    };

    Fields();
    ~Fields();
    Fields(Fields&&) noexcept;
    Fields& operator=(Fields&&) noexcept;

    std::vector<Field> fields;
    Param param;
    Vk::Buffer paramBuffer;
    float step;

    // pipelines/render graph can use these
    bool has_temperature;
    glm::ivec3 lights_dim;
    Lights lights;
    std::unique_ptr<FireLightsUpdater> lights_updater;
    SelfIlluminationLights self_illumination_lights;
    Vk::Image fire_color_img;

    static glm::mat4x4 toLocaluvw(const Camera& camera, const glm::vec3& start_pos, const glm::vec3& size);

    void destroy();
    static Fields fromConfiguration(FieldsConfiguration& cfg);
#ifdef _WIN64
    HANDLE getVkFieldMemHandle(int index);
    HANDLE getVkFieldMemHandle(const std::string& field_name);
#else
    int getVkFieldMemHandle(int index);
    int getVkFieldMemHandle(const std::string& field_name);
#endif

private:
    void initFireLights(FieldsConfiguration& cfg);
    void initFireColorImage(FieldsConfiguration& cfg);
};
