#pragma once

#include "core/config/config.h"
#include "function/global_context.h"
#include "function/physics/physics_engine.h"
#include "function/render/render_engine.h"
#include "function/script/script.h"
#include "function/ui/ui_engine.h"

struct ImDrawData;
#ifdef _WIN64
struct GLFWwindow;
#else
class GLFWwindow;
#endif

class Engine {
    RenderEngine* render_engine;
    UIEngine* ui_engine;
    PhysicsEngine* physics_engine;

    std::vector<std::unique_ptr<Script>> scripts;

    GLFWwindow* window;
    Configuration config;

    std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();

    void update_frame_time();

public:
    void init(Configuration& config,
              RenderEngine* render_engine, UIEngine* ui_engine, PhysicsEngine* physics_engine,
              std::vector<std::unique_ptr<Script>>&& scripts = {});
    void run();
    void cleanup();
};
