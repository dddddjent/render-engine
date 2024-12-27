#pragma once

#include "core/config/config.h"
#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/type/buffer.h"

#ifdef _WIN64
#include <Windows.h>
#endif

struct Object {
    struct Param {
        Vk::DescriptorHandle material;
    };

    std::string name;
    uuid::UUID uuid;

    std::string mesh;
    Param param;
    Vk::Buffer paramBuffer;

#ifdef _WIN64
    HANDLE getVkVertexMemHandle();
#else
    int getVkVertexMemHandle();
#endif
    void destroy();
    static Object fromConfiguration(ObjectConfiguration& config);
};
