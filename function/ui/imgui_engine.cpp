#include "imgui_engine.h"
#include "core/vulkan/vulkan_to_imgui.h"
#include "function/global_context.h"
#include "function/resource_manager/resource_manager.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_stdlib.h"
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

    defaultInit(config);
}

void ImGuiEngine::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiEngine::defaultInit(const Configuration& config)
{
    prev_mouse_delta   = ImVec2(0, 0);
    record_output_path = config.at("recorder").at("output_path");
}

void ImGuiEngine::defaultHandleMouseInput()
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        float dx           = mouse_delta.x - prev_mouse_delta.x;
        float dy           = mouse_delta.y - prev_mouse_delta.y;
        g_ctx.rm->camera.update_rotation(dx, dy);

        prev_mouse_delta = mouse_delta;
    } else {
        prev_mouse_delta = ImVec2(0, 0);
    }
}

void ImGuiEngine::defaultHandleKeyboardInput()
{
    if (is_input_text_focused) {
        return;
    }

    if (ImGui::IsKeyDown(ImGuiKey_W))
        g_ctx.rm->camera.go_forward();
    if (ImGui::IsKeyDown(ImGuiKey_S))
        g_ctx.rm->camera.go_backward();
    if (ImGui::IsKeyDown(ImGuiKey_A))
        g_ctx.rm->camera.go_left();
    if (ImGui::IsKeyDown(ImGuiKey_D))
        g_ctx.rm->camera.go_right();
    if (ImGui::IsKeyDown(ImGuiKey_E))
        g_ctx.rm->camera.go_up();
    if (ImGui::IsKeyDown(ImGuiKey_Q))
        g_ctx.rm->camera.go_down();
}

void ImGuiEngine::defaultRecorderUI()
{
    ImGui::Begin("Recording");

    ImGui::Text("Record Output Path");
    ImGui::SameLine();
    ImGui::InputText("##Record Output Path", &record_output_path);
    if (ImGui::IsItemActive()) {
        is_input_text_focused = true;
    } else {
        is_input_text_focused = false;
    }

    if (g_ctx.rm->recorder.is_recording) {
        if (ImGui::Button("Stop Recording")) {
            g_ctx.rm->recorder.end();
        }
    } else {
        if (ImGui::Button("StartRecording")) {
            g_ctx.rm->recorder.begin(
                record_output_path,
                g_ctx.vk.swapChainImages[0]->extent.width,
                g_ctx.vk.swapChainImages[0]->extent.height);
        }
    }
    ImGui::End();
}

void ImGuiEngine::defaultObjectUI()
{
    ImGui::Begin("Objects");
    for (auto& object : g_ctx.rm->objects) {
        ImGui::Text("%s", object.name.c_str());
        if (ImGui::DragFloat3(
                (std::string("translate##") + object.name).c_str(),
                glm::value_ptr(object.translate), 0.1, -1000000, 1000000)) {
            object.updateTransform();
        }
        if (ImGui::DragFloat3(
                (std::string("rotation##") + object.name).c_str(),
                glm::value_ptr(object.rotate), 1, -1000000, 1000000)) {
            object.updateTransform();
        }
        if (ImGui::DragFloat3(
                (std::string("scale##") + object.name).c_str(),
                glm::value_ptr(object.scale), 0.1, 0.001, 1000000)) {
            object.updateTransform();
        }
    }
    ImGui::End();
}

void ImGuiEngine::defaultMaterialUI()
{
    ImGui::Begin("Materials");
    for (auto& material_pair : g_ctx.rm->materials) {
        auto& material = material_pair.second;
        ImGui::Text("%s", material.name.c_str());
        ImGui::Text("\tmaterial");
        if (ImGui::ColorEdit3(
                (std::string("color##") + material.name).c_str(),
                glm::value_ptr(material.data.color))) {
            material.update(material.data);
        }
        if (ImGui::SliderFloat(
                (std::string("roughness##") + material.name).c_str(),
                &material.data.roughness, 0, 1)) {
            material.update(material.data);
        }
        if (ImGui::SliderFloat(
                (std::string("metallic##") + material.name).c_str(),
                &material.data.metallic, 0, 1)) {
            material.update(material.data);
        }
        ImGui::Text("");
    }
    ImGui::End();
}

void ImGuiEngine::defaultCameraUI()
{
    ImGui::Begin("Camera");
    auto& camera = g_ctx.rm->camera;
    if (ImGui::DragFloat3("position", glm::value_ptr(camera.data.eye_w), 0.03f)) {
        camera.update_position(camera.data.eye_w);
    }
    if (ImGui::DragFloat("yaw", &camera.rotation.x, 1.0f, -180.0f, 180.0f)) {
        camera.update_rotation(camera.rotation);
    }
    if (ImGui::DragFloat("pitch", &camera.rotation.y, 1.0f, -89.0f, 89.0f)) {
        camera.update_rotation(camera.rotation);
    }
    if (ImGui::DragFloat("fov", &camera.data.fov_y, 1, 30, 120)) {
        camera.update_fov(camera.data.fov_y);
    }
    ImGui::End();
}

void ImGuiEngine::defaultLightUI()
{
    ImGui::Begin("Lights");
    auto& lights = g_ctx.rm->lights;
    for (int i = 0; i < lights.data.size(); i++) {
        auto& light = lights.data[i];
        ImGui::Text("%d", i);
        if (ImGui::DragFloat3(
                (std::string("posOrDir##") + std::to_string(i)).c_str(),
                glm::value_ptr(light.posOrDir), 0.03f)) {
            lights.update(&light, i, 1);
        }
        if (ImGui::DragFloat3(
                (std::string("intensity##") + std::to_string(i)).c_str(),
                glm::value_ptr(light.intensity), 0.03f, 0, FLT_MAX)) {
            lights.update(&light, i, 1);
        }
    }
    ImGui::End();
}

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

std::function<void(VkCommandBuffer)> ImGuiEngine::getDrawUIFunction()
{
    return [this](VkCommandBuffer commandBuffer) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        defaultObjectUI();
        defaultMaterialUI();
        defaultCameraUI();
        defaultLightUI();
        defaultRecorderUI();
        drawAxis();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    };
};
