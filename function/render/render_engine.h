#pragma once

#include "core/config/config.h"
#include "core/vulkan/vulkan_to_imgui.h"
#include "function/render/render_graph/render_graph.h"
#include <functional>
#include <memory>
#include <vulkan/vulkan.h>
#ifdef _WIN64
struct GLFWwindow;
#else
class GLFWwindow;
#endif

struct GlobalContext;

class RenderEngine {
public:
    void init_core(const Configuration& config);
    void init_render(const Configuration& config, GlobalContext* g_ctx, std::function<void(VkCommandBuffer)> fn);
    void render();
    GLFWwindow* getGLFWWindow();
    void sync();
    void cleanup();

    void registerImGui(std::function<void(VkCommandBuffer)> fn);
    void* toUI();

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initGLFW();
    void initRenderGraph(std::function<void(VkCommandBuffer)> fn);
    void draw();
    void onResize();

    Configuration* config;
    GlobalContext* g_ctx;
    std::unique_ptr<RenderGraph> render_graph;

    uint32_t WIDTH = 800;
    uint32_t HEIGHT = 600;

    GLFWwindow* window;
    std::string base_window_name;
    std::unique_ptr<Vk2ImGui> vk2im;
    bool framebufferResized = false;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 1;
};
