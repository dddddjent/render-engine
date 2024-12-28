#include "image.h"
#include "core/vulkan/type/buffer.h"
#include "core/vulkan/vulkan_context.h"
#include "core/vulkan/vulkan_util.h"

namespace Vk {
Image::Image()  = default;
Image::~Image() = default;

Image Image::New(const Context& ctx,
                 VkFormat format,
                 VkExtent3D extent,
                 VkImageUsageFlags usage,
                 VkImageAspectFlags aspectFlags,
                 VkMemoryPropertyFlags properties,
                 uint32_t mipLevels,
                 bool external,
                 VkImageTiling tiling,
                 VkImageType imageType,
                 VkImageViewType viewType)
{
    Image i;
    i.CreateUUID();
    i.size    = createImage(ctx, extent, format, usage, properties, i.image, i.memory, external, tiling, imageType, mipLevels);
    i.view    = createImageView(ctx, i.image, format, aspectFlags, viewType, mipLevels);
    i.format  = format;
    i.extent  = extent;
    i.layout  = VK_IMAGE_LAYOUT_UNDEFINED;
    i.sampler = VK_NULL_HANDLE;
    return i;
}

void Image::CreateUUID()
{
    assert(id == uuid::nil_uuid());
    id = uuid::newUUID();
}

void Image::TransitionLayout(const Context& ctx, VkImageLayout newLayout)
{
    transitionImageLayout(ctx.commandBuffer, image, format, layout, newLayout);
    layout = newLayout;
}

void Image::TransitionLayoutSingleTime(const Context& ctx, VkImageLayout newLayout)
{
    transitionImageLayoutSingleTime(ctx, image, format, layout, newLayout);
    layout = newLayout;
}

void Image::AddDefaultSampler(const Context& ctx)
{
    AddSampler(ctx,
               VK_FILTER_LINEAR,
               std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER));
}

void Image::AddSampler(const Context& ctx, const VkFilter filter, const std::vector<VkSamplerAddressMode>& addressMode, const VkBorderColor borderColor)
{
    assert(addressMode.size() == 3);
    VkPhysicalDeviceProperties properties {};
    vkGetPhysicalDeviceProperties(ctx.physicalDevice, &properties);
    VkSamplerCreateInfo sampler_info {};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = filter;
    sampler_info.minFilter               = filter;
    sampler_info.addressModeU            = addressMode[0];
    sampler_info.addressModeV            = addressMode[1];
    sampler_info.addressModeW            = addressMode[2];
    sampler_info.borderColor             = borderColor;
    sampler_info.anisotropyEnable        = VK_TRUE;
    sampler_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable           = VK_FALSE;
    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias              = 0.0f;
    sampler_info.minLod                  = 0.0f;
    sampler_info.maxLod                  = 0.0f;
    if (vkCreateSampler(ctx.device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Image::Update(const Context& ctx, const void* data, uint32_t mipLevel)
{
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    createBuffer(
        ctx,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory, true);
    void* mapped_data;
    vkMapMemory(ctx.device, staging_buffer_memory, 0, size, 0, &mapped_data);
    memcpy(mapped_data, data, size);
    vkUnmapMemory(ctx.device, staging_buffer_memory);

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        TransitionLayoutSingleTime(ctx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    }
    copyBufferToImageSingleTime(ctx, staging_buffer, image, layout, format, extent, mipLevel);

    vkDestroyBuffer(ctx.device, staging_buffer, nullptr);
    vkFreeMemory(ctx.device, staging_buffer_memory, nullptr);
}

void Image::CopyTo(
    const Context& ctx,
    Image& dst,
    const VkExtent3D& extent,
    uint32_t srcMipLevel,
    uint32_t dstMipLevel,
    const VkOffset3D& srcOffset,
    const VkOffset3D& dstOffset) const
{
    if (srcOffset.x + extent.width > this->extent.width
        || srcOffset.y + extent.height > this->extent.height
        || srcOffset.z + extent.depth > this->extent.depth)
        throw std::runtime_error("image overflow");

    if (dstOffset.x + extent.width > dst.extent.width
        || dstOffset.y + extent.height > dst.extent.height
        || dstOffset.z + extent.depth > dst.extent.depth)
        throw std::runtime_error("image overflow");

    copyImageToImage(
        ctx.commandBuffer,
        image,
        dst.image,
        layout,
        dst.layout,
        format,
        dst.format,
        extent,
        srcMipLevel,
        dstMipLevel,
        srcOffset,
        dstOffset);
}

void Image::CopyTo(
    const Context& ctx,
    Buffer& dst,
    const VkExtent3D& extent,
    uint32_t mipLevel,
    const VkOffset3D& srcOffset,
    size_t dstOffset) const
{
    if (srcOffset.x + extent.width > this->extent.width
        || srcOffset.y + extent.height > this->extent.height
        || srcOffset.z + extent.depth > this->extent.depth)
        throw std::runtime_error("image overflow");

    copyImageToBuffer(
        ctx.commandBuffer,
        image,
        dst.buffer,
        layout,
        format,
        extent,
        mipLevel,
        srcOffset,
        dstOffset);
}

void Image::CopyToSingleTime(
    const Context& ctx,
    Image& dst,
    const VkExtent3D& extent,
    uint32_t srcMipLevel,
    uint32_t dstMipLevel,
    const VkOffset3D& srcOffset,
    const VkOffset3D& dstOffset) const
{
    if (srcOffset.x + extent.width > this->extent.width
        || srcOffset.y + extent.height > this->extent.height
        || srcOffset.z + extent.depth > this->extent.depth)
        throw std::runtime_error("image overflow");

    if (dstOffset.x + extent.width > dst.extent.width
        || dstOffset.y + extent.height > dst.extent.height
        || dstOffset.z + extent.depth > dst.extent.depth)
        throw std::runtime_error("image overflow");

    copyImageToImageSingleTime(
        ctx,
        image,
        dst.image,
        layout,
        dst.layout,
        format,
        dst.format,
        extent,
        srcMipLevel,
        dstMipLevel,
        srcOffset,
        dstOffset);
}

void Image::CopyToSingleTime(
    const Context& ctx,
    Buffer& dst,
    const VkExtent3D& extent,
    uint32_t mipLevel,
    const VkOffset3D& srcOffset,
    size_t dstOffset) const
{
    if (srcOffset.x + extent.width > this->extent.width
        || srcOffset.y + extent.height > this->extent.height
        || srcOffset.z + extent.depth > this->extent.depth)
        throw std::runtime_error("image overflow");

    copyImageToBuffer(
        ctx.commandBuffer,
        image,
        dst.buffer,
        layout,
        format,
        extent,
        mipLevel,
        srcOffset,
        dstOffset);
}

void Image::Delete(const Context& ctx, Image& i)
{
    vkDestroyImageView(ctx.device, i.view, nullptr);
    vkDestroyImage(ctx.device, i.image, nullptr);
    vkFreeMemory(ctx.device, i.memory, nullptr);
    if (i.sampler != VK_NULL_HANDLE)
        vkDestroySampler(ctx.device, i.sampler, nullptr);
}
}
