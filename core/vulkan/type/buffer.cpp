#include "buffer.h"
#include "core/vulkan/type/image.h"
#include "core/vulkan/vulkan_context.h"
#include "core/vulkan/vulkan_util.h"

using namespace Vk;

Buffer::Buffer() = default;
Buffer::~Buffer() = default;

Buffer Buffer::New(const Vk::Context& ctx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    bool cpu_mapped,
    bool external)
{
    Buffer b;
    b.CreateUUID();
    b.size = size;
    createBuffer(ctx, size, usage, properties, b.buffer, b.memory, external);
    b.mapped = nullptr;
    if (cpu_mapped)
        vkMapMemory(ctx.device, b.memory, 0, size, 0, &b.mapped);
    b.usage = usage;
    return b;
}

void Buffer::CreateUUID()
{
    assert(id == uuid::nil_uuid());
    id = uuid::newUUID();
}

void Buffer::Update(const Context& ctx, const void* data, size_t size, size_t offset)
{
    if (size + offset > this->size)
        throw std::runtime_error("buffer overflow");

    if (mapped != nullptr) {
        memcpy((char*)mapped + offset, data, size);
        return;
    }

    if ((usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
        throw std::runtime_error("buffer must be created with VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    }

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

    copyBufferSingleTime(ctx, staging_buffer, buffer, size, 0, offset);

    vkDestroyBuffer(ctx.device, staging_buffer, nullptr);
    vkFreeMemory(ctx.device, staging_buffer_memory, nullptr);
}

void Buffer::CopyToSingleTime(
    const Context& ctx,
    Buffer& dst,
    size_t size,
    size_t srcOffset,
    size_t dstOffset) const
{
    if (size + srcOffset > this->size || size + dstOffset > dst.size)
        throw std::runtime_error("buffer overflow");

    copyBufferSingleTime(ctx, dst.buffer, buffer, size, srcOffset, dstOffset);
}

void Buffer::CopyToSingleTime(
    const Context& ctx,
    Image& dst,
    const VkExtent3D& extent,
    uint32_t mipLevel,
    size_t srcOffset,
    const VkOffset3D& dstOffset) const
{
    if (dstOffset.x + extent.width > dst.extent.width
        || dstOffset.y + extent.height > dst.extent.height
        || dstOffset.z + extent.depth > dst.extent.depth)
        throw std::runtime_error("image overflow");

    copyBufferToImageSingleTime(
        ctx,
        buffer,
        dst.image,
        dst.layout,
        dst.format,
        extent,
        mipLevel,
        dstOffset,
        srcOffset);
}

void Buffer::CopyTo(
    const Context& ctx,
    Image& dst,
    const VkExtent3D& extent,
    uint32_t mipLevel,
    size_t srcOffset,
    const VkOffset3D& dstOffset) const
{
    if (dstOffset.x + extent.width > dst.extent.width
        || dstOffset.y + extent.height > dst.extent.height
        || dstOffset.z + extent.depth > dst.extent.depth)
        throw std::runtime_error("image overflow");

    copyBufferToImageSingleTime(
        ctx,
        buffer,
        dst.image,
        dst.layout,
        dst.format,
        extent,
        mipLevel,
        dstOffset,
        srcOffset);
}

void Buffer::CopyTo(
    const Context& ctx,
    Buffer& dst,
    size_t size,
    size_t srcOffset,
    size_t dstOffset) const
{
    if (size + srcOffset > this->size || size + dstOffset > dst.size)
        throw std::runtime_error("buffer overflow");

    copyBuffer(ctx.commandBuffer, buffer, dst.buffer, size, srcOffset, dstOffset);
}

void Buffer::Delete(const Context& ctx, Buffer& b)
{
    vkDestroyBuffer(ctx.device, b.buffer, nullptr);
    vkFreeMemory(ctx.device, b.memory, nullptr);
}
