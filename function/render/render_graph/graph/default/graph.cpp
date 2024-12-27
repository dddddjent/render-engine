#include "graph.h"

void DefaultGraph::init(Configuration& cfg)
{
    nodes["DefaultObject"]
        = std::move(std::make_unique<DefaultObject>("DefaultObject", "object_color", "depth"));
    nodes["HDRToSDR"]
        = std::move(std::make_unique<HDRToSDR>("HDRToSDR", "object_color", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()));
    nodes["UI"]
        = std::move(std::make_unique<UI>("UI", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME(), fn));
    nodes["Record"]
        = std::move(std::make_unique<Record>("Record", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()));
    initAttachments();

    for (auto& node : nodes) {
        node.second->init(cfg, attachments);
    }

    graph = {
        { "HDRToSDR", { "DefaultObject" } },
        { "Record", { "HDRToSDR" } },
        { "UI", { "Record", "HDRToSDR" } },
    };
    initGraph();
}
