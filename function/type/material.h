#pragma once

#include "core/config/config.h"
#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/type/buffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct MaterialData {
    glm::vec3 color;
    float roughness;
    float metallic;
    Vk::DescriptorHandle color_texture;
    Vk::DescriptorHandle metallic_texture;
    Vk::DescriptorHandle roughness_texture;
    Vk::DescriptorHandle normal_texture;
    Vk::DescriptorHandle ao_texture;
};

struct Material {
    std::string name;

    MaterialData data;
    Vk::Buffer buffer;

    void update(const MaterialData& data);
    void destroy();
    static Material fromConfiguration(const MaterialConfiguration& config);
};
