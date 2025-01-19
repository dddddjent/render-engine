#pragma once

#include "function/ui/ui_engine.h"
#include "imgui.h"

struct ImDrawData;

class ImGuiEngine : public UIEngine {
    std::function<void(std::function<void()>)> execSingleTimeCommand;

public:
    ImVec2 prev_mouse_delta;
    std::string record_output_path;
    bool is_input_text_focused = false;

    static void drawAxis();

    void defaultInit(const Configuration& config);
    void defaultHandleMouseInput();
    void defaultHandleKeyboardInput();
    void defaultRecorderUI();
    void defaultObjectUI();
    void defaultMaterialUI();
    void defaultCameraUI();
    void defaultLightUI();

    virtual void init(const Configuration& config, void* render_to_ui) override;
    virtual void cleanup() override;
    virtual std::function<void(VkCommandBuffer)> getDrawUIFunction() override;
    virtual void handleInput() override
    {
        defaultHandleKeyboardInput();
        defaultHandleMouseInput();
    }
};
