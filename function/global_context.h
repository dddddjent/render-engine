#pragma once

#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/vulkan_context.h"

#ifdef _WIN64
struct GLFWwindow;
#else
class GLFWwindow;
#endif
class ResourceManager;

struct GlobalContext {
    GlobalContext();
    ~GlobalContext();

    void init(Configuration& config, GLFWwindow* window);
    void cleanup();

    Vk::Context vk;
    Vk::DescriptorManager dm;
    std::unique_ptr<ResourceManager> rm;

    float frame_time = 0.0f;
    uint32_t currentFrame = 0;

    bool continue_to_run = true;
};

extern GlobalContext g_ctx;
