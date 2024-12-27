#pragma once

#include "function/ui/ui_engine.h"

struct ImDrawData;

class ImGuiEngine : public UIEngine {
    std::function<void(std::function<void()>)> execSingleTimeCommand;

public:
    static void drawAxis();

    virtual void init(const Configuration& config, void* render_to_ui) override;
    virtual void cleanup() override;
    virtual std::function<void(VkCommandBuffer)> getDrawUIFunction() override;
    virtual void handleInput() override { }
};
