#pragma once

#include "function/render/render_graph/node/node.h"
#include "function/render/render_graph/render_graph.h"

class VorticityFieldGraph : public RenderGraph {
    std::function<void(VkCommandBuffer)> fn;

public:
    void init(Configuration& cfg) override;
    VkRenderPass getUIRenderpass() override
    {
        return dynamic_cast<UI*>(nodes["UI"].get())->render_pass;
    }
    void registerUIRenderfunction(std::function<void(VkCommandBuffer)> fn) override
    {
        this->fn = fn;
    }
};
