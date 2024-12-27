#pragma once

#include "core/config/config.h"
#include "core/tool/logger.h"
#include "core/vulkan/debug_messager.h"
#include "core/vulkan/queue_family_indices.h"
#include <vulkan/vulkan.h>
#ifdef _WIN64
// Don't define min() and max()
#define NOMINMAX
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifdef _WIN64
struct GLFWwindow;
#else
class GLFWwindow;
#endif

namespace Vk {
struct Image;

struct Context {
    Context();
    ~Context();

    void init(const Configuration& config, GLFWwindow* window);
    void recreateSwapChain();
    void cleanup();

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    DebugMessager debugMessager;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkQueue queue;
    VkQueue presentQueue;
    QueueFamilyIndices queueFamilyIndices;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain;
    std::vector<std::unique_ptr<Image>> swapChainImages;

    VkSemaphore cuUpdateSemaphore, vkUpdateSemaphore;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
#ifdef _WIN64
    HANDLE cuUpdateSemaphoreHandle;
    HANDLE vkUpdateSemaphoreHandle;
#else
    int cuUpdateSemaphoreFd;
    int vkUpdateSemaphoreFd;
#endif

private:
    void initVulkan();

    void createInstance();

    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice();
    void createLogicalDeviceAndQueue();

    void createSurface();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    void createSwapChainImageViews();
    void cleanupSwapChain();

    void createCommandPoolAndBuffer();

    void createSyncObjects();
    void createSyncObjectsExt();

    uint32_t WIDTH = 800;
    uint32_t HEIGHT = 600;
    const int MAX_FRAMES_IN_FLIGHT = 1;
    uint32_t currentFrame = 0;

    const std::vector<const char*> instanceExtensions = {
#ifdef DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
#endif
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
#ifdef _WIN64
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
#endif

#ifdef DEBUG
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#endif
    };

#ifdef DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
};
}
