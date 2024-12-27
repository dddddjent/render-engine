#pragma once

#include "function/render/render_graph/render_graph_node.h"

class UI : public RenderGraphNode {
    void createRenderPass();
    void createFramebuffer();

    std::vector<VkFramebuffer> framebuffers;
    RenderAttachments* attachments;
    std::function<void(VkCommandBuffer)> fn;

public:
    UI(
        const std::string& name,
        const std::string& color_buf_name,
        std::function<void(VkCommandBuffer)> fn);

    virtual void init(Configuration& cfg, RenderAttachments& attachments) override;
    virtual void record(uint32_t swapchain_index) override;
    virtual void onResize() override;
    virtual void destroy() override;

    VkRenderPass render_pass;
};
