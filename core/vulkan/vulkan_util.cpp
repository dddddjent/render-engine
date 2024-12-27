#include "core/vulkan/vulkan_util.h"
#include "core/tool/sh.h"
#include "core/vulkan/vulkan_context.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN64
#include <VersionHelpers.h>
#include <aclapi.h>
#include <dxgi1_2.h>
#include <windows.h>
#define _USE_MATH_DEFINES
#endif

namespace Vk {

#ifdef _WIN64
PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32Handle;
PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32Handle;
#else
PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR;
PFN_vkGetSemaphoreFdKHR fpGetSemaphoreFdKHR;

#endif

uint32_t findMemoryType(const Context& ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void singleTimeCommands(const Context& ctx, const std::function<void(const VkCommandBuffer&)>& fn)
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    fn(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(ctx.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.queue);

    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

VkDeviceSize createImage(
    const Context& ctx,
    const VkExtent3D& extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory,
    bool external,
    const VkImageTiling tiling,
    const VkImageType imageType,
    const uint32_t mipLevels)
{
    VkExternalMemoryImageCreateInfo externalImageInfo = {};
    externalImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#ifdef _WIN64
    externalImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    externalImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (external)
        imageInfo.pNext = &externalImageInfo;

    if (vkCreateImage(ctx.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

#ifdef _WIN64
    WindowsSecurityAttributes winSecurityAttributes;

    VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
    vulkanExportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
    vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
    vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
    vulkanExportMemoryWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
    vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;
#endif
    VkExportMemoryAllocateInfo exportMemoryInfo = {};
    exportMemoryInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN64
    exportMemoryInfo.pNext = &vulkanExportMemoryWin32HandleInfoKHR;
    exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ctx.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(ctx, memRequirements.memoryTypeBits, properties);
    if (external)
        allocInfo.pNext = &exportMemoryInfo;

    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(ctx.device, image, imageMemory, 0);

    return memRequirements.size;
}

VkImageView createImageView(
    const Vk::Context& ctx,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkImageViewType viewType,
    const uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void createBuffer(
    const Vk::Context& ctx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory,
    bool external)
{
    VkExternalMemoryBufferCreateInfo externalBufferInfo = {};
    externalBufferInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
#ifdef _WIN64
    externalBufferInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    externalBufferInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

#ifdef _WIN64
    WindowsSecurityAttributes winSecurityAttributes;

    VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
    vulkanExportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
    vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
    vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
    vulkanExportMemoryWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
    vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;
#endif
    VkExportMemoryAllocateInfo exportMemoryInfo = {};
    exportMemoryInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
#ifdef _WIN64
    exportMemoryInfo.pNext = &vulkanExportMemoryWin32HandleInfoKHR;
    exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    // buffer
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (external)
        bufferInfo.pNext = &externalBufferInfo;
    if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(ctx, memRequirements.memoryTypeBits, properties);
    if (external)
        allocInfo.pNext = &exportMemoryInfo;
    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    vkBindBufferMemory(ctx.device, buffer, bufferMemory, 0);
}

void copyBufferSingleTime(
    const Context& ctx,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkDeviceSize srcOffset,
    VkDeviceSize dstOffset)
{
    singleTimeCommands(ctx, [&](const VkCommandBuffer& commandBuffer) {
        copyBuffer(commandBuffer, srcBuffer, dstBuffer, size, srcOffset, dstOffset);
    });
}

void copyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkDeviceSize srcOffset,
    VkDeviceSize dstOffset)
{
    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

struct LayoutDependency {
    LayoutDependency(uint32_t access, uint32_t stage)
        : access(static_cast<VkAccessFlagBits>(access))
        , stage(static_cast<VkPipelineStageFlagBits>(stage))
    {
    }
    VkAccessFlagBits access;
    VkPipelineStageFlagBits stage;
};

static std::unordered_map<VkImageLayout, LayoutDependency> layoutDependencies = {
    { VK_IMAGE_LAYOUT_UNDEFINED,
        LayoutDependency {
            0,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT } },

    { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT } },

    { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT } },

    { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } },

    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT } },

    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT } },

    { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        LayoutDependency {
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT } },

    { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        LayoutDependency {
            VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT } },

    { VK_IMAGE_LAYOUT_GENERAL,
        LayoutDependency {
            VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT } },
};

void transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO

    assert(format != VK_FORMAT_D32_SFLOAT_S8_UINT && format != VK_FORMAT_D24_UNORM_S8_UINT);

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT
        || format == VK_FORMAT_D24_UNORM_S8_UINT
        || format == VK_FORMAT_D32_SFLOAT) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    const auto sourceDependency = layoutDependencies.find(oldLayout);
    const auto destinationDependency = layoutDependencies.find(newLayout);
    if (sourceDependency == layoutDependencies.end())
        throw std::runtime_error("Unsupported old layout");
    if (destinationDependency == layoutDependencies.end())
        throw std::runtime_error("Unsupported new layout");
    barrier.srcAccessMask |= sourceDependency->second.access;
    barrier.dstAccessMask |= destinationDependency->second.access;
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceDependency->second.stage,
        destinationDependency->second.stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void transitionImageLayoutSingleTime(
    const Vk::Context& ctx,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    singleTimeCommands(ctx, [&](const VkCommandBuffer& commandBuffer) {
        transitionImageLayout(commandBuffer, image, format, oldLayout, newLayout);
    });
}

void copyBufferToImage(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkImage image,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel,
    const VkOffset3D& imageOffset,
    const VkDeviceSize& bufferOffset)
{
    transitionImageLayout(
        commandBuffer,
        image, format,
        imageLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region {};
    region.bufferOffset = bufferOffset; // Offset into the buffer (if needed)
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // If it's a color image
    region.imageSubresource.mipLevel = mipLevel; // Mipmap level
    region.imageSubresource.baseArrayLayer = 0; // Starting layer (for 3D image, usually 0)
    region.imageSubresource.layerCount = 1; // Only one "layer" for 3D images

    region.imageOffset = imageOffset;
    region.imageExtent = extent;
    if (imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    // Copy the buffer data into the 3D image
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Ensure image is in transfer destination layout
        1,
        &region);

    transitionImageLayout(
        commandBuffer,
        image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout);
}

void copyBufferToImageSingleTime(
    const Context& ctx,
    VkBuffer buffer,
    VkImage image,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel,
    const VkOffset3D& imageOffset,
    const VkDeviceSize& bufferOffset)
{
    singleTimeCommands(ctx, [&](const VkCommandBuffer& commandBuffer) {
        copyBufferToImage(commandBuffer, buffer, image, imageLayout, format, extent, mipLevel, imageOffset, bufferOffset);
    });
}

void copyImageToBuffer(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkBuffer buffer,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel,
    const VkOffset3D& imageOffset,
    const VkDeviceSize& bufferOffset)
{
    transitionImageLayout(
        commandBuffer,
        image,
        format,
        imageLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region = {};
    region.bufferOffset = bufferOffset;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = imageOffset;
    region.imageExtent = extent;
    if (imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    transitionImageLayout(commandBuffer,
        image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        imageLayout);
}

void copyImageToBufferSingleTime(
    const Context& ctx,
    VkImage image,
    VkBuffer buffer,
    VkImageLayout imageLayout,
    VkFormat format,
    const VkExtent3D& extent,
    const uint32_t mipLevel,
    const VkOffset3D& imageOffset,
    const VkDeviceSize& bufferOffset)
{
    singleTimeCommands(ctx, [&](const VkCommandBuffer& commandBuffer) {
        copyImageToBuffer(commandBuffer, image, buffer, imageLayout, format, extent, mipLevel, imageOffset, bufferOffset);
    });
}

void copyImageToImage(
    VkCommandBuffer commandBuffer,
    VkImage src,
    VkImage dst,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkFormat srcFormat,
    VkFormat dstFormat,
    const VkExtent3D& extent,
    const uint32_t srcMipLevel,
    const uint32_t dstMipLevel,
    const VkOffset3D& srcOffset,
    const VkOffset3D& dstOffset)
{
    transitionImageLayout(
        commandBuffer,
        src,
        srcFormat,
        srcLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transitionImageLayout(
        commandBuffer,
        dst,
        dstFormat,
        dstLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageCopy region = {};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel = srcMipLevel;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = srcOffset;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel = dstMipLevel;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = dstOffset;
    region.extent = extent;
    if (srcLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || srcLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    vkCmdCopyImage(commandBuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImageLayout(
        commandBuffer,
        src,
        srcFormat,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        srcLayout);
    transitionImageLayout(
        commandBuffer,
        dst,
        dstFormat,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        dstLayout);
}

void copyImageToImageSingleTime(
    const Context& ctx,
    VkImage src,
    VkImage dst,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkFormat srcFormat,
    VkFormat dstFormat,
    const VkExtent3D& extent,
    const uint32_t srcMipLevel,
    const uint32_t dstMipLevel,
    const VkOffset3D& srcOffset,
    const VkOffset3D& dstOffset)
{
    singleTimeCommands(ctx, [&](const VkCommandBuffer& commandBuffer) {
        copyImageToImage(commandBuffer, src, dst, srcLayout, dstLayout, srcFormat, dstFormat, extent, srcMipLevel, dstMipLevel, srcOffset, dstOffset);
    });
}

VkShaderModule createShaderModule(const Context& ctx, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

VkExtent3D toVkExtent3D(const VkExtent2D& extent)
{
    return { extent.width, extent.height, 1 };
}

VkExtent2D toVkExtent2D(const VkExtent3D& extent)
{
    assert(extent.depth == 1);
    return { extent.width, extent.height };
}

#ifdef _WIN64
HANDLE getVkSemaphoreHandle(const Context& ctx, VkSemaphore& semaphore)
{
    HANDLE handle;

    VkSemaphoreGetWin32HandleInfoKHR vulkanSemaphoreGetWin32HandleInfoKHR = {};
    vulkanSemaphoreGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    vulkanSemaphoreGetWin32HandleInfoKHR.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    vulkanSemaphoreGetWin32HandleInfoKHR.semaphore = semaphore;

    fpGetSemaphoreWin32Handle(ctx.device, &vulkanSemaphoreGetWin32HandleInfoKHR, &handle);

    return handle;
}
#else
int getVkSemaphoreHandle(const Context& ctx, VkSemaphore& semaphore)
{
    int fd;

    VkSemaphoreGetFdInfoKHR vulkanSemaphoreGetFdInfoKHR = {};
    vulkanSemaphoreGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    vulkanSemaphoreGetFdInfoKHR.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    vulkanSemaphoreGetFdInfoKHR.semaphore = semaphore;

    auto result = fpGetSemaphoreFdKHR(ctx.device, &vulkanSemaphoreGetFdInfoKHR, &fd);

    return fd;
}
#endif

#ifdef _WIN64
WindowsSecurityAttributes::WindowsSecurityAttributes()
{
    m_winPSecurityDescriptor = (PSECURITY_DESCRIPTOR)calloc(
        1, SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void**));

    PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
    PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

    InitializeSecurityDescriptor(m_winPSecurityDescriptor,
        SECURITY_DESCRIPTOR_REVISION);

    SID_IDENTIFIER_AUTHORITY sidIdentifierAuthority = SECURITY_WORLD_SID_AUTHORITY;
    AllocateAndInitializeSid(&sidIdentifierAuthority, 1, SECURITY_WORLD_RID, 0, 0,
        0, 0, 0, 0, 0, ppSID);

    EXPLICIT_ACCESS explicitAccess;
    ZeroMemory(&explicitAccess, sizeof(EXPLICIT_ACCESS));
    explicitAccess.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
    explicitAccess.grfAccessMode = SET_ACCESS;
    explicitAccess.grfInheritance = INHERIT_ONLY;
    explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    explicitAccess.Trustee.ptstrName = (LPTSTR)*ppSID;

    SetEntriesInAcl(1, &explicitAccess, NULL, ppACL);

    SetSecurityDescriptorDacl(m_winPSecurityDescriptor, TRUE, *ppACL, FALSE);

    m_winSecurityAttributes.nLength = sizeof(m_winSecurityAttributes);
    m_winSecurityAttributes.lpSecurityDescriptor = m_winPSecurityDescriptor;
    m_winSecurityAttributes.bInheritHandle = TRUE;
}

SECURITY_ATTRIBUTES* WindowsSecurityAttributes::operator&()
{
    return &m_winSecurityAttributes;
}

WindowsSecurityAttributes::~WindowsSecurityAttributes()
{
    PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
    PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

    if (*ppSID) {
        FreeSid(*ppSID);
    }
    if (*ppACL) {
        LocalFree(*ppACL);
    }
    free(m_winPSecurityDescriptor);
}
#endif
}
