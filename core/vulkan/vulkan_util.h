#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <string>
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

struct Context;

uint32_t findMemoryType(
    const Vk::Context& ctx,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties);

void singleTimeCommands(
    const Vk::Context& ctx,
    const std::function<void(const VkCommandBuffer&)>& fn);

void createBuffer(
    const Vk::Context& ctx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory,
    bool external = false);

void copyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkDeviceSize srcOffset = 0,
    VkDeviceSize dstOffset = 0);
void copyBufferSingleTime(
    const Context& ctx,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkDeviceSize srcOffset = 0,
    VkDeviceSize dstOffset = 0);

VkDeviceSize createImage(
    const Context& ctx,
    const VkExtent3D& extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory,
    bool external = false,
    const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
    const VkImageType imageType = VK_IMAGE_TYPE_2D,
    const uint32_t mipLevels = 1);
VkImageView createImageView(
    const Vk::Context& ctx,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
    const uint32_t mipLevels = 1);

void transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);
void transitionImageLayoutSingleTime(
    const Vk::Context& ctx,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);

void copyBufferToImage(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkImage image,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel = 0,
    const VkOffset3D& imageOffset = { 0, 0, 0 },
    const VkDeviceSize& bufferOffset = 0);
void copyBufferToImageSingleTime(
    const Context& ctx,
    VkBuffer buffer,
    VkImage image,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel = 0,
    const VkOffset3D& imageOffset = { 0, 0, 0 },
    const VkDeviceSize& bufferOffset = 0);
void copyImageToBuffer(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkBuffer buffer,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel = 0,
    const VkOffset3D& imageOffset = { 0, 0, 0 },
    const VkDeviceSize& bufferOffset = 0);
void copyImageToBufferSingleTime(
    const Context& ctx,
    VkImage image,
    VkBuffer buffer,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel = 0,
    const VkOffset3D& imageOffset = { 0, 0, 0 },
    const VkDeviceSize& bufferOffset = 0);
void copyImageToImage(
    VkCommandBuffer commandBuffer,
    VkImage src,
    VkImage dst,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkFormat srcFormat,
    VkFormat dstFormat,
    const VkExtent3D& extent,
    const uint32_t srcMipLevel = 0,
    const uint32_t dstMipLevel = 0,
    const VkOffset3D& srcOffset = { 0, 0, 0 },
    const VkOffset3D& dstOffset = { 0, 0, 0 });
void copyImageToImageSingleTime(
    const Context& ctx,
    VkImage src,
    VkImage dst,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkFormat srcFormat,
    VkFormat dstFormat,
    const VkExtent3D& extent,
    const uint32_t srcMipLevel = 0,
    const uint32_t dstMipLevel = 0,
    const VkOffset3D& srcOffset = { 0, 0, 0 },
    const VkOffset3D& dstOffset = { 0, 0, 0 });

VkShaderModule createShaderModule(const Vk::Context& ctx, const std::vector<char>& code);

VkExtent3D toVkExtent3D(const VkExtent2D& extent);
VkExtent2D toVkExtent2D(const VkExtent3D& extent);

#ifdef _WIN64
extern PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32Handle;
extern PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32Handle;
HANDLE getVkSemaphoreHandle(const Context& ctx, VkSemaphore& semaphore);
#else
extern PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR;
extern PFN_vkGetSemaphoreFdKHR fpGetSemaphoreFdKHR;
int getVkSemaphoreHandle(const Context& ctx, VkSemaphore& semaphore);
#endif

#ifdef _WIN64
class WindowsSecurityAttributes {
protected:
    SECURITY_ATTRIBUTES m_winSecurityAttributes;
    PSECURITY_DESCRIPTOR m_winPSecurityDescriptor;

public:
    WindowsSecurityAttributes();
    SECURITY_ATTRIBUTES* operator&();
    ~WindowsSecurityAttributes();
};
#endif
}
