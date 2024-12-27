#include "graph.h"

void FireFieldGraph::init(Configuration& cfg)
{
    nodes["FireObject"]
        = std::move(std::make_unique<FireObject>("FireObject", "object_color", "depth"));
    nodes["FireField"]
        = std::move(std::make_unique<FireFieldNode>("FireField", "object_color", "depth", "field_object_color"));
    nodes["HDRToSDR"]
        = std::move(std::make_unique<HDRToSDR>("HDRToSDR", "field_object_color", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()));
    nodes["UI"]
        = std::move(std::make_unique<UI>("UI", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME(), fn));
    nodes["Record"]
        = std::move(std::make_unique<Record>("Record", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()));
    initAttachments();

    for (auto& node : nodes) {
        node.second->init(cfg, attachments);
    }

    graph = {
        { "FireField", { "FireObject" } },
        { "HDRToSDR", { "FireField" } },
        { "Record", { "HDRToSDR" } },
        { "UI", { "Record", "HDRToSDR" } },
    };
    initGraph();
}
