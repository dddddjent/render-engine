#include "engine.h"
#include "core/tool/logger.h"
#include <GLFW/glfw3.h>
#include <function/resource_manager/resource_manager.h>

void Engine::init(Configuration& config,
                  RenderEngine* render_engine, UIEngine* ui_engine, PhysicsEngine* physics_engine,
                  std::vector<std::unique_ptr<Script>>&& scripts,
                  std::unique_ptr<RenderGraph> render_graph)
{
    logger.init(config);
    INFO_ALL("Initializing the engine...");

    this->render_engine  = render_engine;
    this->ui_engine      = ui_engine;
    this->physics_engine = physics_engine;
    this->scripts        = std::move(scripts);

    render_engine->init_core(config);
    window = render_engine->getGLFWWindow();

    for (auto& script : this->scripts) {
        script->before_g_ctx(config);
    }
    g_ctx.init(config, window);

    for (auto& script : this->scripts) {
        script->before_internal_engine_init(config);
    }
    if(render_graph != nullptr) {
        INFO_ALL("Using custom render graph");
    }
    render_engine->init_render(config, &g_ctx, ui_engine->getDrawUIFunction(), std::move(render_graph));
    physics_engine->init(config, &g_ctx);

    ui_engine->init(config, render_engine->toUI()); // get renderpass from render graph

    for (auto& script : this->scripts) {
        script->init(config);
    }

    INFO_ALL("Engine Initialized");
}

void Engine::update_frame_time()
{
    float deltaTime  = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - currentTime).count();
    g_ctx.frame_time = deltaTime;

    currentTime = std::chrono::high_resolution_clock::now();
}

void Engine::run()
{
    while (!glfwWindowShouldClose(window)) {
        INFO_FILE("Engine loop started");

        glfwPollEvents();

        update_frame_time();

        ui_engine->handleInput();
        render_engine->render();
        physics_engine->step();

        for (auto& script : scripts) {
            script->step(g_ctx.frame_time);
        }

        INFO_FILE("Engine loop ended");

        if (g_ctx.continue_to_run == false)
            break;

        g_ctx.currentFrame++;
    }
}

void Engine::cleanup()
{
    INFO_FILE("Engine cleanup started");

    render_engine->sync();
    physics_engine->sync();

    for (auto& script : scripts) {
        script->destroy();
    }

    physics_engine->cleanup();
    ui_engine->cleanup();
    render_engine->cleanup();
    g_ctx.cleanup();

    INFO_FILE("Engine cleanup ended");
}
