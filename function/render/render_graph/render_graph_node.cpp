#include "render_graph_node.h"
#include "function/global_context.h"

RenderGraphNode::RenderGraphNode(const std::string& name)
    : name(name)
{
}

VkRenderPass RenderGraphNode::DefaultRenderPass(
    std::unordered_map<std::string, RenderAttachmentDescription>& attachment_descriptions,
    const std::vector<AttachmentDescriptionHelper>& desc,
    VkSubpassDependency& dependency)
{
    std::vector<VkAttachmentDescription> attachments;
    for (const auto& d : desc) {
        attachments.push_back(
            {
                .format = attachment_descriptions[d.name].format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = static_cast<VkAttachmentLoadOp>(d.load_op),
                .storeOp = static_cast<VkAttachmentStoreOp>(d.store_op),
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = attachment_descriptions[d.name].layout,
                .finalLayout = attachment_descriptions[d.name].layout,
            });
    }

    bool has_depth_stencil = false;
    std::vector<VkAttachmentReference> colorAttachmentRefs {};
    VkAttachmentReference depthAttachmentRef {};
    for (const auto& d : desc) {
        if (static_cast<uint8_t>(attachment_descriptions[d.name].type & RenderAttachmentType::Depth) != 0) {
            assert(!has_depth_stencil);
            has_depth_stencil = true;
            depthAttachmentRef = {
                .attachment = static_cast<uint32_t>(colorAttachmentRefs.size()),
                .layout = attachment_descriptions[d.name].layout,
            };
        } else {
            colorAttachmentRefs.push_back(
                {
                    .attachment = static_cast<uint32_t>(colorAttachmentRefs.size()),
                    .layout = attachment_descriptions[d.name].layout,
                });
        }
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = has_depth_stencil ? &depthAttachmentRef : nullptr;

    VkRenderPass render_pass;
    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(g_ctx.vk.device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    return render_pass;
}

void RenderGraphNode::bindDescriptorSet(uint32_t index, VkPipelineLayout layout, VkDescriptorSet* set)
{
    vkCmdBindDescriptorSets(
        g_ctx.vk.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        index,
        1,
        set,
        0,
        nullptr);
}

void RenderGraphNode::setDefaultViewportAndScissor()
{
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(g_ctx.vk.swapChainImages[0]->extent.width);
    viewport.height = static_cast<float>(g_ctx.vk.swapChainImages[0]->extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(g_ctx.vk.commandBuffer, 0, 1, &viewport);
    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = Vk::toVkExtent2D(g_ctx.vk.swapChainImages[0]->extent);
    vkCmdSetScissor(g_ctx.vk.commandBuffer, 0, 1, &scissor);
}
