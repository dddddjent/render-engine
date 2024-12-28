#pragma once

#include "core/config/config.h"
#include <functional>
#include <vulkan/vulkan_core.h>

class UIEngine {
public:
    virtual void init(const Configuration& config, void* render_to_ui) = 0;
    virtual void cleanup() = 0;
    virtual std::function<void(VkCommandBuffer)> getDrawUIFunction() = 0;

    virtual void handleInput() = 0;
};
