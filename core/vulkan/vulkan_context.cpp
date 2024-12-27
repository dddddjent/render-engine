#include "vulkan_context.h"
#include "core/vulkan/swapchain_support.h"
#include "core/vulkan/type/image.h"
#include "core/vulkan/vulkan_util.h"
#include <GLFW/glfw3.h>
#include <set>
#ifdef _WIN64
#include <dxgi1_2.h>
#endif

namespace Vk {
Context::Context() = default;
Context::~Context() = default;

void Context::init(const Configuration& config, GLFWwindow* window)
{
    this->window = window;

    initVulkan();
}

void Context::cleanup()
{
    vkDestroySemaphore(device, cuUpdateSemaphore, nullptr);
    vkDestroySemaphore(device, vkUpdateSemaphore, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    cleanupSwapChain();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
#ifdef DEBUG
    debugMessager.destroy(*this);
#endif
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Context::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Simulation";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for (const auto& e : instanceExtensions)
        extensions.push_back(e);

    const VkBool32 setting_validate_sync = VK_TRUE;
    const VkLayerSettingEXT settings[] = {
        { validationLayers[0], "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_sync },
    };
    const VkLayerSettingsCreateInfoEXT layer_settings_create_info = {
        VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr,
        static_cast<uint32_t>(std::size(settings)), settings
    };

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = &layer_settings_create_info;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

#ifdef _WIN64
    fpGetMemoryWin32Handle = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(
        instance, "vkGetMemoryWin32HandleKHR");
    if (fpGetMemoryWin32Handle == NULL) {
        throw std::runtime_error(
            "Vulkan: Proc address for \"vkGetMemoryWin32HandleKHR\" not "
            "found.\n");
    }
#else
    fpGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetInstanceProcAddr(
        instance, "vkGetMemoryFdKHR");
    if (fpGetMemoryFdKHR == NULL) {
        throw std::runtime_error(
            "Vulkan: Proc address for \"vkGetMemoryFdKHR\" not found.\n");
    }
#endif
}

bool Context::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

bool Context::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool Context::isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupport swapChainSupport = SwapChainSupport::querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    } else {
        WARN_CONSOLE("Some of the device extensions are not supported");
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && deviceFeatures.geometryShader
        && deviceFeatures.samplerAnisotropy
        && indices.isComplete()
        && extensionsSupported
        && swapChainAdequate;
}

void Context::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Context::createLogicalDeviceAndQueue()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures {};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.pNext = nullptr;

    VkPhysicalDeviceFeatures2 deviceFeatures {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &descriptorIndexingFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);
    assert(descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind);
    assert(descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind);
    assert(descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing);
    assert(descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind);

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &deviceFeatures;
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &queue);
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

void Context::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

VkSurfaceFormatKHR Context::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Context::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Context::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Context::createSwapChain()
{
    SwapChainSupport swapChainSupport = SwapChainSupport::querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t indices[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data());
    for (int i = 0; i < imageCount; i++) {
        swapChainImages.emplace_back(std::make_unique<Image>());

        auto& swapChainImage = swapChainImages.back();
        swapChainImage->image = images[i];
        swapChainImage->CreateUUID();
        swapChainImage->size = extent.width * extent.height * 4; // unorm
        swapChainImage->format = surfaceFormat.format;
        swapChainImage->extent = VkExtent3D { extent.width, extent.height, 1 };
        swapChainImage->TransitionLayoutSingleTime(*this, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}

void Context::createSwapChainImageViews()
{
    for (int i = 0; i < swapChainImages.size(); i++) {
        swapChainImages[i]->view = createImageView(
            *this,
            swapChainImages[i]->image,
            swapChainImages[i]->format,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Context::cleanupSwapChain()
{
    for (auto& swapChainImage : swapChainImages) {
        vkDestroyImageView(device, swapChainImage->view, nullptr);
        swapChainImage.reset(nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    swapChainImages.clear();
}

void Context::recreateSwapChain()
{
    cleanupSwapChain();

    createSwapChain();
    createSwapChainImageViews();
}

void Context::createCommandPoolAndBuffer()
{
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Context::initVulkan()
{
    createInstance();
#ifdef DEBUG
    debugMessager.init(*this);
#endif
    createSurface();
    pickPhysicalDevice();
    queueFamilyIndices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    createLogicalDeviceAndQueue();
    createCommandPoolAndBuffer();

    createSwapChain();
    createSwapChainImageViews();

    createSyncObjects();
    createSyncObjectsExt();
}

void Context::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void Context::createSyncObjectsExt()
{
#ifdef _WIN64
    fpGetSemaphoreWin32Handle = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
        device, "vkGetSemaphoreWin32HandleKHR");
#else
    fpGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
        device, "vkGetSemaphoreFdKHR");
#endif

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // TODO: ???
    memset(&semaphoreInfo, 0, sizeof(semaphoreInfo));
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

#ifdef _WIN64
    WindowsSecurityAttributes winSecurityAttributes;

    VkExportSemaphoreWin32HandleInfoKHR vulkanExportSemaphoreWin32HandleInfoKHR = {};
    vulkanExportSemaphoreWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
    vulkanExportSemaphoreWin32HandleInfoKHR.pNext = NULL;
    vulkanExportSemaphoreWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
    vulkanExportSemaphoreWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
    vulkanExportSemaphoreWin32HandleInfoKHR.name = (LPCWSTR)NULL;
#endif

    VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
    vulkanExportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

#ifdef _WIN64
    vulkanExportSemaphoreCreateInfo.pNext = &vulkanExportSemaphoreWin32HandleInfoKHR;
    vulkanExportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    vulkanExportSemaphoreCreateInfo.pNext = NULL;
    vulkanExportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    semaphoreInfo.pNext = &vulkanExportSemaphoreCreateInfo;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
            &cuUpdateSemaphore)
            != VK_SUCCESS
        || vkCreateSemaphore(device, &semaphoreInfo, nullptr,
               &vkUpdateSemaphore)
            != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create synchronization objects for a CUDA-Vulkan!");
    }

#ifdef _WIN64
    cuUpdateSemaphoreHandle = getVkSemaphoreHandle(*this, cuUpdateSemaphore);
    vkUpdateSemaphoreHandle = getVkSemaphoreHandle(*this, vkUpdateSemaphore);
#else
    cuUpdateSemaphoreFd = getVkSemaphoreHandle(*this, cuUpdateSemaphore);
    vkUpdateSemaphoreFd = getVkSemaphoreHandle(*this, vkUpdateSemaphore);
#endif
}

}
