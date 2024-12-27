#include "render_graph.h"
#include <queue>

void RenderGraph::clearAttachments()
{
    for (auto& attachment : attachments.attachments) {
        auto layout = attachment.second.image.layout;
        attachment.second.image.TransitionLayout(g_ctx.vk, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkImageSubresourceRange range = {};
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        if (static_cast<uint8_t>(attachment.second.type & RenderAttachmentType::Color) != 0) {
            VkClearColorValue clearColor = {};
            clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vkCmdClearColorImage(
                g_ctx.vk.commandBuffer,
                attachment.second.image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
        }
        if (static_cast<uint8_t>(attachment.second.type & RenderAttachmentType::Depth) != 0) {
            VkClearDepthStencilValue clearValue = {};
            clearValue = { 1.0f, 0 };
            range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vkCmdClearDepthStencilImage(
                g_ctx.vk.commandBuffer,
                attachment.second.image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
        }
        attachment.second.image.TransitionLayout(g_ctx.vk, layout);
    }
}

void RenderGraph::initGraph()
{
    for (const auto& node : nodes) {
        in_degree[node.first] = 0;
    }
    for (const auto& edge : graph) {
        in_degree[edge.first] = edge.second.size();
        for (const auto& node : edge.second)
            rev_graph[node].emplace_back(edge.first);
    }
    for (const auto& node : nodes) {
        if (in_degree[node.first] == 0) {
            starting_nodes.emplace_back(node.first);
        }
    }
}

void RenderGraph::initAttachments()
{
    std::unordered_map<std::string, RenderAttachmentDescription> descriptions;
    for (const auto& node : nodes) {
        for (auto& desc_pair : node.second->attachment_descriptions) {
            if (desc_pair.second.name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME())
                continue;

            auto it = descriptions.find(desc_pair.second.name);
            if (it == descriptions.end()) {
                descriptions[desc_pair.second.name] = desc_pair.second;
            } else {
                assert(it->second.format == desc_pair.second.format);
                it->second.usage |= desc_pair.second.usage;
                it->second.type = it->second.type | desc_pair.second.type;
                it->second.rw = it->second.rw | desc_pair.second.rw;
            }
        }
    }
    for (const auto& desc : descriptions) {
        attachments.addAttachment(desc.first, desc.second.type, desc.second.usage, desc.second.format);
    }
}

void RenderGraph::prepareAttachmentsForNode(const auto& node, uint32_t swapchain_index)
{
    for (const auto& desc_pair : node->attachment_descriptions) {
        auto& image = desc_pair.second.name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()
            ? *g_ctx.vk.swapChainImages[swapchain_index]
            : attachments.getAttachment(desc_pair.second.name);
        if (image.layout == swapchain_index)
            continue;
        image.TransitionLayout(g_ctx.vk, desc_pair.second.layout);
    }
}

void RenderGraph::record(uint32_t swapchain_index)
{
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    if (vkBeginCommandBuffer(g_ctx.vk.commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    clearAttachments();

    auto degree = in_degree;
    std::queue<std::string> queue;
    for (const auto& node : starting_nodes) {
        queue.emplace(node);
    }
    while (!queue.empty()) {
        std::string name = queue.front();
        auto& node = nodes[name];
        queue.pop();
        degree[name] = -1;

        {
            prepareAttachmentsForNode(node, swapchain_index);
            node->record(swapchain_index);
        }

        for (const auto& next_node : rev_graph[name]) {
            if (degree[next_node] == -1)
                continue;
            degree[next_node]--;
            if (degree[next_node] == 0) {
                queue.emplace(next_node);
            }
        }
    }
    g_ctx.vk.swapChainImages[swapchain_index]->TransitionLayout(g_ctx.vk, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    if (vkEndCommandBuffer(g_ctx.vk.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void RenderGraph::onResize()
{
    attachments.onResize();
    for (auto& node : nodes) {
        node.second->onResize();
    }
}

void RenderGraph::destroy()
{
    for (auto& node : nodes)
        node.second->destroy();
    attachments.cleanup();
}
