#pragma once

#include "function/render/render_graph/render_graph_node.h"

class FireObject : public RenderGraphNode {
    struct Param {
        Vk::DescriptorHandle camera;
        Vk::DescriptorHandle lights;
        Vk::DescriptorHandle fire_lights;
    };

    void createRenderPass();
    void createFramebuffer();
    void createPipeline(Configuration& cfg);

    Pipeline<Param> pipeline;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    RenderAttachments* attachments;

public:
    FireObject(
        const std::string& name,
        const std::string& color_buf_name,
        const std::string& depth_buf_name);

    virtual void init(Configuration& cfg, RenderAttachments& attachments) override;
    virtual void record(uint32_t swapchain_index) override;
    virtual void onResize() override;
    virtual void destroy() override;
};
