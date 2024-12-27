#pragma once

#include "function/render/render_graph/render_graph_node.h"

class HDRToSDR : public RenderGraphNode {
    struct Param {
        Vk::DescriptorHandle hdr_img;
    };

    void createRenderPass();
    void createFramebuffer();
    void createPipeline(Configuration& cfg);

    Pipeline<Param> pipeline;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    RenderAttachments* attachments;

public:
    HDRToSDR(
        const std::string& name,
        const std::string& hdr_buf,
        const std::string& sdr_buf);

    virtual void init(Configuration& cfg, RenderAttachments& attachments) override;
    virtual void record(uint32_t swapchain_index) override;
    virtual void onResize() override;
    virtual void destroy() override;
};
