#include "graph.h"

void VorticityFieldGraph::init(Configuration& cfg)
{
    nodes["DefaultObject"]
        = std::move(std::make_unique<DefaultObject>("DefaultObject", "object_color", "depth"));
    nodes["VorticityField"]
        = std::move(std::make_unique<VorticityFieldNode>("VorticityField", "object_color", "depth", "field_object_color"));
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
        { "VorticityField", { "DefaultObject" } },
        { "HDRToSDR", { "VorticityField" } },
        { "CalculateLuminance", { "HDRToSDR" } },
        { "FXAA", { "CalculateLuminance" } },
        { "Record", { "FXAA" } },
        { "UI", { "Record", "HDRToSDR" } },
    };
    RenderGraph::initGraph();
}
