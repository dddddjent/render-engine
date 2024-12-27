#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#ifdef _WIN64
struct GLFWwindow;
#else
class GLFWwindow;
#endif

struct Vk2ImGui {
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkDescriptorPool descriptorPool;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkQueue queue;
    uint32_t queueFamily;
    int image_count;
    std::function<void(std::function<void()>)> execSingleTimeCommand;
};
