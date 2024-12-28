#pragma once

#include "core/tool/uuid.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vk {
struct Context;
struct Buffer;

struct Image {
    Image();
    ~Image();

    static Image New(const Context& ctx,
                     VkFormat format,
                     VkExtent3D extent,
                     VkImageUsageFlags usage,
                     VkImageAspectFlags aspectFlags,
                     VkMemoryPropertyFlags properties,
                     uint32_t mipLevels       = 1,
                     bool external            = false,
                     VkImageTiling tiling     = VK_IMAGE_TILING_OPTIMAL,
                     VkImageType imageType    = VK_IMAGE_TYPE_2D,
                     VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    static void Delete(const Context& ctx, Image& i);

    void CreateUUID();
    void AddSampler(const Context& ctx, const VkFilter filter, const std::vector<VkSamplerAddressMode>& addressMode, const VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
    void AddDefaultSampler(const Context& ctx);
    void Update(const Context& ctx, const void* data, uint32_t mipLevel = 0);
    void TransitionLayout(const Context& ctx, VkImageLayout newLayout);
    void TransitionLayoutSingleTime(const Context& ctx, VkImageLayout newLayout);
    void CopyTo(
        const Context& ctx,
        Image& dst,
        const VkExtent3D& extent,
        uint32_t srcMipLevel        = 0,
        uint32_t dstMipLevel        = 0,
        const VkOffset3D& srcOffset = { 0, 0, 0 },
        const VkOffset3D& dstOffset = { 0, 0, 0 }) const;
    void CopyTo(
        const Context& ctx,
        Buffer& dst,
        const VkExtent3D& extent,
        uint32_t mipLevel           = 0,
        const VkOffset3D& srcOffset = { 0, 0, 0 },
        size_t dstOffset            = 0) const;
    // No need to submit command buffer
    void CopyToSingleTime(
        const Context& ctx,
        Image& dst,
        const VkExtent3D& extent,
        uint32_t srcMipLevel        = 0,
        uint32_t dstMipLevel        = 0,
        const VkOffset3D& srcOffset = { 0, 0, 0 },
        const VkOffset3D& dstOffset = { 0, 0, 0 }) const;
    // No need to submit command buffer
    void CopyToSingleTime(
        const Context& ctx,
        Buffer& dst,
        const VkExtent3D& extent,
        uint32_t mipLevel           = 0,
        const VkOffset3D& srcOffset = { 0, 0, 0 },
        size_t dstOffset            = 0) const;

    uuid::UUID id = uuid::nil_uuid();
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkExtent3D extent;
    VkFormat format;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    size_t size;

    VkSampler sampler = VK_NULL_HANDLE;
};
}
