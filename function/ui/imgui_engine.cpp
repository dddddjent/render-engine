#include "imgui_engine.h"
#include "core/vulkan/vulkan_to_imgui.h"
#include "function/global_context.h"
#include "function/resource_manager/resource_manager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void ImGuiEngine::init(const Configuration& config, void* render_to_ui)
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    auto vk2im = static_cast<Vk2ImGui*>(render_to_ui);
    ImGui_ImplGlfw_InitForVulkan(vk2im->window, true);

    ImGui_ImplVulkan_InitInfo info;
    info.Instance            = vk2im->instance;
    info.PhysicalDevice      = vk2im->physicalDevice;
    info.Device              = vk2im->device;
    info.Queue               = vk2im->queue;
    info.QueueFamily         = vk2im->queueFamily;
    info.DescriptorPool      = vk2im->descriptorPool;
    info.RenderPass          = vk2im->renderPass;
    info.Subpass             = vk2im->subpass;
    info.MinImageCount       = vk2im->image_count;
    info.ImageCount          = vk2im->image_count;
    info.MinAllocationSize   = 1024 * 1024;
    info.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
    info.PipelineCache       = nullptr;
    info.Allocator           = nullptr;
    info.CheckVkResultFn     = check_vk_result;
    info.UseDynamicRendering = false;
    ImGui_ImplVulkan_Init(&info);
}

void ImGuiEngine::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

std::function<void(VkCommandBuffer)> ImGuiEngine::getDrawUIFunction()
{
    return [](VkCommandBuffer commandBuffer) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // TODO: draw widgets
        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    };
};

void ImGuiEngine::drawAxis()
{
    static constexpr glm::vec3 origin(0, 0, 0);
    static constexpr glm::vec3 points[] = {
        glm::vec3(1, 0, 0), // x
        glm::vec3(0, 1, 0), // y
        glm::vec3(0, 0, 1), // z
    };
    static constexpr ImU32 colors[] = {
        IM_COL32(255, 0, 0, 255),
        IM_COL32(0, 255, 0, 255),
        IM_COL32(0, 0, 255, 255),
    };
    constexpr float scale  = 50.0f;
    constexpr float offset = 100.0f;

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 screen_size    = ImGui::GetIO().DisplaySize;

    glm::mat4 proj = glm::lookAt(-g_ctx.rm->camera.data.view_dir, origin, g_ctx.rm->camera.data.up);

    glm::vec4 temp = proj * glm::vec4(origin, 1.0f);
    temp /= temp.w;
    glm::vec3 origin_point = glm::vec3(temp);
    origin_point.x += offset;
    origin_point.y += screen_size.y - offset;

    for (int i = 0; i < 3; i++) {
        temp = proj * glm::vec4(points[i], 1.0f);
        temp /= temp.w;
        glm::vec3 point = glm::vec3(temp);
        point.y         = -point.y;
        point *= scale;
        point.x += offset;
        point.y += screen_size.y - offset;

        draw_list->AddLine(
            ImVec2(origin_point.x, origin_point.y),
            ImVec2(point.x, point.y),
            colors[i], 3.0f);
    }
}
