#pragma once

#include <functional>
#include <vulkan/vulkan.h>

namespace Vk {
struct Context;

class DebugMessager {
    std::function<VkResult(VkInstance,
        const VkDebugUtilsMessengerCreateInfoEXT*,
        const VkAllocationCallbacks*,
        VkDebugUtilsMessengerEXT*)>
        vkCreateDebugUtilsMessengerEXT;
    std::function<void(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*)>
        vkDestroyDebugUtilsMessengerEXT;

    VkDebugUtilsMessengerEXT debugMessenger;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

public:
    void init(const Context& ctx);
    void destroy(const Context& ctx);
};
}
