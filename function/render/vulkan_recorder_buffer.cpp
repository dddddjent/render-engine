// #include "vulkan_recorder_buffer.h"
// #include "core/vulkan/vulkan_context.h"
// #include "core/vulkan/vulkan_util.h"
//
// using namespace Vk;
//
// void VulkanRecorderBuffer::init(VkDeviceSize bufferSize)
// {
//     createBuffer(
//         ctx,
//         bufferSize,
//         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//         buffer,
//         memory);
//     data.resize(bufferSize);
//     vkMapMemory(ctx.device, memory, 0, bufferSize, 0, &mapped_data);
// }
//
// void VulkanRecorderBuffer::getData(
//     VkDevice device,
//     VkCommandBuffer commandBuffer,
//     VkImage image,
//     VkFormat format,
//     VkExtent2D extent)
// {
//     transitionImageLayout(commandBuffer,
//         image,
//         format,
//         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
//
//     VkBufferImageCopy region = {};
//     region.bufferOffset = 0;
//     region.bufferRowLength = 0; // Tightly packed
//     region.bufferImageHeight = 0; // Tightly packed
//     region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     region.imageSubresource.mipLevel = 0;
//     region.imageSubresource.baseArrayLayer = 0;
//     region.imageSubresource.layerCount = 1;
//     region.imageOffset = { 0, 0, 0 };
//     region.imageExtent = { extent.width, extent.height, 1 };
//
//     vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
//
//     transitionImageLayout(commandBuffer,
//         image,
//         format,
//         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//
//     memcpy(data.data(), mapped_data, extent.width * extent.height * 4);
// }
//
// void VulkanRecorderBuffer::destroy()
// {
//     vkUnmapMemory(ctx.device, memory);
//     vkDestroyBuffer(ctx.device, buffer, nullptr);
//     vkFreeMemory(ctx.device, memory, nullptr);
// }
