#pragma once

#include "core/config/config.h"
#include "core/vulkan/type/buffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct LightData {
    glm::vec3 posOrDir;
    float padding0;
    glm::vec3 intensity;
    float padding1;
};

struct Lights {
    std::string name;

    std::vector<LightData> data;
    Vk::Buffer buffer;

    void update(const LightData* data, int index, int cnt);
    void destroy();
    static Lights fromConfiguration(const std::vector<LightConfiguration>& config);
};
