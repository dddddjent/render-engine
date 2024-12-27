#include "graph.h"

void SmokeFieldGraph::init(Configuration& cfg)
{
    nodes["DefaultObject"]
        = std::move(std::make_unique<DefaultObject>("DefaultObject", "object_color", "depth"));
    nodes["SmokeField"]
        = std::move(std::make_unique<SmokeFieldNode>("SmokeField", "object_color", "depth", "field_object_color"));
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
        { "SmokeField", { "DefaultObject" } },
        { "HDRToSDR", { "SmokeField" } },
        { "Record", { "HDRToSDR" } },
        { "UI", { "Record", "HDRToSDR" } },
    };
    RenderGraph::initGraph();
}
