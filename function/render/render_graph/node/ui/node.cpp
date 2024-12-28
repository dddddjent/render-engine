#include "./node.h"
#include "core/vulkan/vulkan_util.h"
#include "function/global_context.h"

using namespace Vk;

UI::UI(const std::string& name, const std::string& color_buf_name, std::function<void(VkCommandBuffer)> fn)
    : RenderGraphNode(name)
{
    bool is_swapchain       = color_buf_name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME();
    attachment_descriptions = {
        {
            "color",
            {
                is_swapchain ? RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME() : color_buf_name,
                RenderAttachmentType::Color,
                RenderAttachmentRW::Write,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                is_swapchain ? g_ctx.vk.swapChainImages[0]->format : VK_FORMAT_B8G8R8A8_UNORM,
            },
        },
    };

    this->fn = fn;
}

void UI::init(Configuration& cfg, RenderAttachments& attachments)
{
    this->attachments = &attachments;
    createRenderPass();
    createFramebuffer();
}

void UI::createFramebuffer()
{
    framebuffers.resize(g_ctx.vk.swapChainImages.size());
    for (int i = 0; i < g_ctx.vk.swapChainImages.size(); i++) {
        std::array<VkImageView, 1> views = {
            attachment_descriptions["color"].name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()
                ? g_ctx.vk.swapChainImages[i]->view
                : attachments->getAttachment(attachment_descriptions["color"].name).view
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
        framebufferInfo.pAttachments    = views.data();
        framebufferInfo.width           = g_ctx.vk.swapChainImages[i]->extent.width;
        framebufferInfo.height          = g_ctx.vk.swapChainImages[i]->extent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(g_ctx.vk.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void UI::createRenderPass()
{
    std::vector<AttachmentDescriptionHelper> helpers = {
        { "color", VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE },
    };
    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass = DefaultRenderPass(attachment_descriptions, helpers, dependency);
}

void UI::record(uint32_t swapchain_index)
{
    setDefaultViewportAndScissor();

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = render_pass;
    renderPassInfo.framebuffer       = framebuffers[swapchain_index];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = toVkExtent2D(g_ctx.vk.swapChainImages[swapchain_index]->extent);
    renderPassInfo.clearValueCount   = 0;
    renderPassInfo.pClearValues      = nullptr;
    vkCmdBeginRenderPass(g_ctx.vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    fn(g_ctx.vk.commandBuffer);

    vkCmdEndRenderPass(g_ctx.vk.commandBuffer);
}

void UI::onResize()
{
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_ctx.vk.device, framebuffer, nullptr);
    }
    createFramebuffer();
}

void UI::destroy()
{
    vkDestroyRenderPass(g_ctx.vk.device, render_pass, nullptr);
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_ctx.vk.device, framebuffer, nullptr);
    }
}
