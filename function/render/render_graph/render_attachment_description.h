#pragma once

#include "core/tool/enum_bit_op.h"
#include "function/render/render_graph/render_attachments.h"

enum class RenderAttachmentRW : uint8_t {
    Read  = 1 << 0,
    Write = 1 << 1,
};
DEFINE_ENUM_BIT_OPERATORS(RenderAttachmentRW)

struct RenderAttachmentDescription {
    static constexpr std::string SWAPCHAIN_IMAGE_NAME() { return "swapchain"; }

    std::string name;

    RenderAttachmentType type;
    RenderAttachmentRW rw;
    VkImageLayout layout;
    VkImageUsageFlags usage;
    VkFormat format;
};
