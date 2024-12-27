#pragma once

#include "function/render/render_graph/node/node.h"
#include "function/render/render_graph/render_attachments.h"
#include "function/render/render_graph/render_graph_node.h"
#include <memory>
#include <string>
#include <unordered_map>

struct Configuration;

class RenderGraph {
protected:
    std::unordered_map<std::string, std::unique_ptr<RenderGraphNode>> nodes;
    RenderAttachments attachments;

    // node to dependent nodes
    std::unordered_map<std::string, std::vector<std::string>> graph;
    std::unordered_map<std::string, std::vector<std::string>> rev_graph;
    std::unordered_map<std::string, int> in_degree;
    std::vector<std::string> starting_nodes;

    virtual void clearAttachments();
    void initGraph();
    void prepareAttachmentsForNode(const auto& node, uint32_t swapchain_index);
    void initAttachments();

public:
    virtual ~RenderGraph() = default;
    virtual void init(Configuration& cfg) = 0;
    // record all the render commands
    virtual void record(uint32_t swapchain_index);
    virtual void onResize();
    virtual void destroy();

    // after init
    virtual VkRenderPass getUIRenderpass() { return nullptr; }
    // before init
    virtual void registerUIRenderfunction(std::function<void(VkCommandBuffer)> fn) { }
};
