#include "graph.h"

void FireFieldGraph::init(Configuration& cfg)
{
    nodes["FireObject"]
        = std::move(std::make_unique<FireObject>("FireObject", "object_color", "depth"));
    nodes["FireField"]
        = std::move(std::make_unique<FireFieldNode>("FireField", "object_color", "depth", "field_object_color"));
    nodes["HDRToSDR"]
        = std::move(std::make_unique<HDRToSDR>("HDRToSDR", "field_object_color", "sdr_buf"));
    nodes["CalculateLuminance"]
        = std::move(std::make_unique<CalculateLuminance>("CalculateLuminance", "sdr_buf", "sdr_buf_alpha_illuminance"));
    nodes["FXAA"]
        = std::move(std::make_unique<FXAANode>("FXAA", "sdr_buf_alpha_illuminance", RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME()));
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
        { "CalculateLuminance", { "HDRToSDR" } },
        { "FXAA", { "CalculateLuminance" } },
        { "Record", { "FXAA" } },
        { "UI", { "Record", "FXAA" } },
    };
    initGraph();
}
