#include "./node.h"
#include "core/vulkan/vulkan_util.h"
#include "function/global_context.h"
#include "function/resource_manager/resource_manager.h"

using namespace Vk;

Record::Record(const std::string& name, const std::string& color_buf_name)
    : RenderGraphNode(name)
{
    bool is_swapchain       = color_buf_name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME();
    attachment_descriptions = {
        {
            "color",
            {
                is_swapchain ? RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME() : color_buf_name,
                RenderAttachmentType::Color,
                RenderAttachmentRW::Read,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                is_swapchain ? g_ctx.vk.swapChainImages[0]->format : VK_FORMAT_B8G8R8A8_UNORM,
            },
        },
    };
}

void Record::init(Configuration& cfg, RenderAttachments& attachments)
{
    this->attachments = &attachments;
    createBuffer();
}

void Record::createBuffer()
{
    const auto& image = attachment_descriptions["color"].name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()
        ? *g_ctx.vk.swapChainImages[0]
        : this->attachments->getAttachment(attachment_descriptions["color"].name);

    buffer = Buffer::New(
        g_ctx.vk,
        image.size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);

    data.resize(image.extent.width * image.extent.height * 4);
}

void Record::record(uint32_t swapchain_index)
{
    if (g_ctx.rm->recorder.is_recording) {
        const auto& image = attachment_descriptions["color"].name == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()
            ? *g_ctx.vk.swapChainImages[swapchain_index]
            : this->attachments->getAttachment(attachment_descriptions["color"].name);

        image.CopyTo(g_ctx.vk, buffer, image.extent);
        memcpy(data.data(), buffer.mapped, image.extent.width * image.extent.height * 4);

        g_ctx.rm->recorder.append(data);
    }
}

void Record::onResize()
{
    Buffer::Delete(g_ctx.vk, buffer);
    createBuffer();
}

void Record::destroy()
{
    Buffer::Delete(g_ctx.vk, buffer);
}
