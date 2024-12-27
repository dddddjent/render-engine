#pragma once

#include "function/render/render_graph/pipeline.hpp"
#include "function/render/render_graph/render_attachment_description.h"
#include <string>
#include <unordered_map>

struct Configuration;

class RenderGraphNode {
protected:
    struct AttachmentDescriptionHelper {
        std::string name;
        VkAttachmentLoadOp load_op;
        VkAttachmentStoreOp store_op;
    };
    static VkRenderPass DefaultRenderPass(
        std::unordered_map<std::string, RenderAttachmentDescription>& attachment_descriptions,
        const std::vector<AttachmentDescriptionHelper>& desc,
        VkSubpassDependency& dependency);
    void bindDescriptorSet(uint32_t index, VkPipelineLayout layout, VkDescriptorSet* set);
    void setDefaultViewportAndScissor();

public:
    // init the descriptrion directly in the derived class
    RenderGraphNode(const std::string& name);
    virtual ~RenderGraphNode() = default;

    virtual void init(Configuration& cfg, RenderAttachments& attachments) = 0;
    virtual void record(uint32_t swapchain_index) = 0;
    virtual void onResize() = 0;
    virtual void destroy() = 0;

    std::string name;
    std::unordered_map<std::string, RenderAttachmentDescription> attachment_descriptions;
};
