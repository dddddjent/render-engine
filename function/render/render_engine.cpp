#include "render_engine.h"
#include "core/tool/logger.h"
#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/vulkan_context.h"
#include "function/global_context.h"
#include "function/render/render_graph/graph/graph.h"
#include "function/resource_manager/resource_manager.h"
#include <GLFW/glfw3.h>

using namespace Vk;

void RenderEngine::init_core(const Configuration& config)
{
    WIDTH            = config["width"];
    HEIGHT           = config["height"];
    base_window_name = config["name"];
    this->config     = &const_cast<Configuration&>(config);

    initGLFW();
}

void RenderEngine::init_render(const Configuration& config, GlobalContext* g_ctx,
                               std::function<void(VkCommandBuffer)> fn, std::unique_ptr<RenderGraph> custom_render_graph)
{
    this->g_ctx = g_ctx;
    initRenderGraph(fn, std::move(custom_render_graph));
}

void RenderEngine::initRenderGraph(std::function<void(VkCommandBuffer)> fn, std::unique_ptr<RenderGraph> custom_render_graph)
{
    JSON_GET(RenderGraphConfiguration, render_graph_cfg, (*config), "render_graph");

    if (custom_render_graph != nullptr) { // use custom render graph
        render_graph = std::move(custom_render_graph);
        render_graph->registerUIRenderfunction(fn);
        render_graph->init(*config);
        return;
    }

    if (render_graph_cfg.name == "default") {
        render_graph = std::make_unique<DefaultGraph>();
    } else if (render_graph_cfg.name == "smoke_field") {
        render_graph = std::make_unique<SmokeFieldGraph>();
    } else if (render_graph_cfg.name == "fire_field") {
        render_graph = std::make_unique<FireFieldGraph>();
    } else if (render_graph_cfg.name == "vorticity_field") {
        render_graph = std::make_unique<VorticityFieldGraph>();
    } else {
        throw std::runtime_error("render graph not found: " + render_graph_cfg.name);
    }
    render_graph->registerUIRenderfunction(fn);
    render_graph->init(*config);
}

void RenderEngine::render()
{
    draw();

    auto name = base_window_name + " " + std::to_string(g_ctx->frame_time * 1000) + "ms";
    glfwSetWindowTitle(window, name.c_str());
}

void RenderEngine::draw()
{
    vkWaitForFences(g_ctx->vk.device, 1, &g_ctx->vk.inFlightFences[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT], VK_TRUE, UINT64_MAX);

    uint32_t swapchain_index;
    VkResult result = vkAcquireNextImageKHR(
        g_ctx->vk.device,
        g_ctx->vk.swapChain,
        UINT64_MAX,
        g_ctx->vk.imageAvailableSemaphores[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT],
        VK_NULL_HANDLE,
        &swapchain_index);
    while (result == VK_ERROR_OUT_OF_DATE_KHR) {
        onResize();
        vkWaitForFences(
            g_ctx->vk.device, 1,
            &g_ctx->vk.inFlightFences[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT],
            VK_TRUE, UINT64_MAX);
        result = vkAcquireNextImageKHR(
            g_ctx->vk.device, g_ctx->vk.swapChain,
            UINT64_MAX,
            g_ctx->vk.imageAvailableSemaphores[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT],
            VK_NULL_HANDLE,
            &swapchain_index);
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(g_ctx->vk.device, 1, &g_ctx->vk.inFlightFences[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT]);

    vkResetCommandBuffer(g_ctx->vk.commandBuffer, 0);

    {
        render_graph->record(swapchain_index);
    }

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkSemaphore> waitSemaphores = { g_ctx->vk.imageAvailableSemaphores[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT] };
    if (g_ctx->currentFrame != 0)
        waitSemaphores.emplace_back(g_ctx->vk.cuUpdateSemaphore);
    std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    if (g_ctx->currentFrame != 0)
        waitStages.emplace_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores    = waitSemaphores.data();
    submitInfo.pWaitDstStageMask  = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &g_ctx->vk.commandBuffer;

    VkSemaphore signalSemaphores[] = {
        g_ctx->vk.renderFinishedSemaphores[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT],
        g_ctx->vk.vkUpdateSemaphore
    };
    submitInfo.signalSemaphoreCount = 2;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(g_ctx->vk.queue, 1, &submitInfo, g_ctx->vk.inFlightFences[g_ctx->currentFrame % MAX_FRAMES_IN_FLIGHT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = { g_ctx->vk.swapChain };
    VkPresentInfoKHR presentInfo {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapChains;
    presentInfo.pImageIndices      = &swapchain_index;
    result                         = vkQueuePresentKHR(g_ctx->vk.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        onResize();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void RenderEngine::registerImGui(std::function<void(VkCommandBuffer)> fn)
{
    render_graph->registerUIRenderfunction(fn);
}

void* RenderEngine::toUI()
{
    vk2im                 = std::make_unique<Vk2ImGui>();
    vk2im->window         = window;
    vk2im->instance       = g_ctx->vk.instance;
    vk2im->physicalDevice = g_ctx->vk.physicalDevice;
    vk2im->device         = g_ctx->vk.device;
    vk2im->descriptorPool = g_ctx->dm.uiPool;
    vk2im->renderPass     = render_graph->getUIRenderpass();
    vk2im->subpass        = 0;
    vk2im->image_count    = 2; // imgui requires it to be >= 2
    vk2im->queue          = g_ctx->vk.queue;
    vk2im->queueFamily    = g_ctx->vk.queueFamilyIndices.graphicsFamily.value();
    return (void*)vk2im.get();
}

GLFWwindow* RenderEngine::getGLFWWindow()
{
    return window;
}

void RenderEngine::sync()
{
    vkDeviceWaitIdle(g_ctx->vk.device);
}

void RenderEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto re                = reinterpret_cast<RenderEngine*>(glfwGetWindowUserPointer(window));
    re->framebufferResized = true;
}

void RenderEngine::initGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, base_window_name.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void RenderEngine::onResize()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(g_ctx->vk.device);

    g_ctx->vk.recreateSwapChain();
    render_graph->onResize();
    g_ctx->rm->camera.update_aspect_ratio(
        g_ctx->vk.swapChainImages[0]->extent.width,
        g_ctx->vk.swapChainImages[0]->extent.height);
}

void RenderEngine::cleanup()
{
    render_graph->destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}
