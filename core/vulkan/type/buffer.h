#pragma once

#include "core/tool/uuid.h"
#include <vulkan/vulkan_core.h>

namespace Vk {
struct Context;
struct Image;

struct Buffer {
    Buffer();
    ~Buffer();

    static Buffer New(const Vk::Context& ctx,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        bool cpu_mapped = false,
        bool external = false);
    static void Delete(const Vk::Context& ctx, Buffer& b);
    void CreateUUID();
    void Update(const Context& ctx, const void* data, size_t size, size_t offset = 0);
    void CopyTo(
        const Context& ctx,
        Buffer& dst,
        size_t size,
        size_t srcOffset = 0,
        size_t dstOffset = 0) const;
    void CopyTo(
        const Context& ctx,
        Image& dst,
        const VkExtent3D& extent,
        uint32_t mipLevel = 0,
        size_t srcOffset = 0,
        const VkOffset3D& dstOffset = { 0, 0, 0 }) const;
    // No need to submit command buffer
    void CopyToSingleTime(
        const Context& ctx,
        Buffer& dst,
        size_t size,
        size_t srcOffset = 0,
        size_t dstOffset = 0) const;
    // No need to submit command buffer
    void CopyToSingleTime(
        const Context& ctx,
        Image& dst,
        const VkExtent3D& extent,
        uint32_t mipLevel = 0,
        size_t srcOffset = 0,
        const VkOffset3D& dstOffset = { 0, 0, 0 }) const;

    uuid::UUID id = uuid::nil_uuid();
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped = nullptr;
    VkBufferUsageFlags usage;
    size_t size = 0;
};
}
