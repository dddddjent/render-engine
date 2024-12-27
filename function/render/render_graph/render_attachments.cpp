#include "render_attachments.h"
#include "core/vulkan/type/image.h"
#include "function/global_context.h"
#include "render_attachment_description.h"

using namespace Vk;

void RenderAttachment::destroy()
{
    Vk::Image::Delete(g_ctx.vk, image);
}

VkImageAspectFlags RenderAttachments::getAspectFlags(RenderAttachmentType type)
{
    assert(type != RenderAttachmentType::Sampler); // must be more than sampler
    VkImageAspectFlags aspectFlags = 0;
    if (static_cast<uint8_t>(type & RenderAttachmentType::Color) != 0) {
        aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (static_cast<uint8_t>(type & RenderAttachmentType::Depth) != 0) {
        aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return aspectFlags;
}

void RenderAttachments::addAttachment(const std::string& name, RenderAttachmentType type, VkImageUsageFlags usage, VkFormat format)
{
    assert(name != RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME());
    RenderAttachment attachment;
    attachment.name = name;
    attachment.type = type;
    attachment.usage = usage;
    attachment.image = Image::New(
        g_ctx.vk,
        format,
        g_ctx.vk.swapChainImages[0]->extent,
        usage,
        getAspectFlags(type),
        VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    attachment.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_GENERAL);
    if (static_cast<uint8_t>(type & RenderAttachmentType::Sampler) != 0) {
        attachment.image.AddDefaultSampler(g_ctx.vk);
        g_ctx.dm.registerResource(attachment.image, DescriptorType::CombinedImageSampler);
    }
    attachments[name] = std::move(attachment);
}

void RenderAttachments::removeAttachment(const std::string& name)
{
    assert(name != RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME());
    auto& attachment = attachments[name];
    attachment.destroy();
    attachments.erase(name);
}

Vk::Image& RenderAttachments::getAttachment(const std::string& name)
{
    assert(name != RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME());
    auto it = attachments.find(name);
    if (it == attachments.end()) {
        throw std::runtime_error("Attachment not found: " + name);
    }
    return it->second.image;
}
void RenderAttachments::onResize()
{
    for (auto& a : attachments) {
        auto id = a.second.image.id;
        if (a.second.image.sampler != VK_NULL_HANDLE)
            g_ctx.dm.removeResourceRegistration(id);
        Image::Delete(g_ctx.vk, a.second.image);

        a.second.image = Image::New(
            g_ctx.vk,
            a.second.image.format,
            g_ctx.vk.swapChainImages[0]->extent,
            a.second.usage,
            getAspectFlags(a.second.type),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        a.second.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_GENERAL);
        a.second.image.id = id;
        if (static_cast<uint8_t>(a.second.type & RenderAttachmentType::Sampler) != 0) {
            a.second.image.AddDefaultSampler(g_ctx.vk);
            g_ctx.dm.registerResource(a.second.image, DescriptorType::CombinedImageSampler);
        }
    }
}

void RenderAttachments::cleanup()
{
    for (auto& a : attachments) {
        a.second.destroy();
    }
}
