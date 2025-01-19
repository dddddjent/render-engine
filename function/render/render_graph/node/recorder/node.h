#pragma once

#include "function/render/render_graph/render_graph_node.h"

// left to right (width), top to bottom (height), B8G8R8A8_UINT8
// brga, brga....
class Record : public RenderGraphNode {
    Vk::Buffer buffer;
    RenderAttachments* attachments;
    std::vector<uint8_t> data;
    bool is_recording_prev = false;

    void createBuffer();

public:
    Record(
        const std::string& name,
        const std::string& color_buf_name);

    virtual void init(Configuration& cfg, RenderAttachments& attachments) override;
    virtual void record(uint32_t swapchain_index) override;
    virtual void onResize() override;
    virtual void destroy() override;
};
