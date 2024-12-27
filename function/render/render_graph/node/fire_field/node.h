#pragma once

#include "function/render/render_graph/render_graph_node.h"

class FireFieldNode : public RenderGraphNode {
    struct Param {
        Vk::DescriptorHandle camera;
        Vk::DescriptorHandle lights;
        Vk::DescriptorHandle self_illumination_lights;
        Vk::DescriptorHandle fire_color;
        Vk::DescriptorHandle previous_color;
        Vk::DescriptorHandle previous_depth;
    };

    void createRenderPass();
    void createFramebuffer();
    void createPipeline(Configuration& cfg);

    Pipeline<Param> pipeline;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    RenderAttachments* attachments;

public:
    FireFieldNode(
        const std::string& name,
        const std::string& previous_color,
        const std::string& previous_depth,
        const std::string& color_buf);

    virtual void init(Configuration& cfg, RenderAttachments& attachments) override;
    virtual void record(uint32_t swapchain_index) override;
    virtual void onResize() override;
    virtual void destroy() override;
};
