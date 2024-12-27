// #pragma once
//
// #include <cstdint>
// #include <cstring>
// #include <vector>
// #include <vulkan/vulkan_core.h>
//
// struct VulkanRecorderBuffer {
//     // left to right (width), top to bottom (height), B8G8R8A8_UINT8
//     // brga, brga....
//     std::vector<uint8_t> data;
//     VkBuffer buffer;
//     VkDeviceMemory memory;
//     void* mapped_data;
//
//     void init(VkDeviceSize bufferSize);
//     void getData(
//         VkDevice device,
//         VkCommandBuffer commandBuffer,
//         VkImage image,
//         VkFormat format,
//         VkExtent2D extent);
//     void destroy();
// };
