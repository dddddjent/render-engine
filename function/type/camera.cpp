#include "camera.h"
#include "core/math/math.h"
#include "core/tool/logger.h"
#include "function/global_context.h"
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

// windows.h has to be after after quaternion
#include "core/vulkan/vulkan_util.h"

using namespace Vk;

void Camera::destroy()
{
    Buffer::Delete(g_ctx.vk, buffer);
}

void Camera::update_position(const glm::vec3& position)
{
    glm::vec3 center = position + data.view_dir;
    data.eye_w = position;
    data.view = glm::lookAt(position, center, data.up);
    buffer.Update(g_ctx.vk, &data, sizeof(CameraData));
}

void Camera::update_view_dir(const glm::vec3& view_dir)
{
    data.view_dir = glm::normalize(view_dir);
    data.view = glm::lookAt(data.eye_w, data.eye_w + data.view_dir, data.up);
    buffer.Update(g_ctx.vk, &data, sizeof(CameraData));
}

void Camera::update_up(const glm::vec3& up)
{
    data.view = glm::lookAt(data.eye_w, data.eye_w + data.view_dir, up);
    data.up = up;
    buffer.Update(g_ctx.vk, &data, sizeof(CameraData));
}

void Camera::update_fov(const float fov)
{
    data.proj = glm::perspective(glm::radians(fov),
        g_ctx.vk.swapChainImages[0]->extent.width / (float)g_ctx.vk.swapChainImages[0]->extent.height,
        0.1f, 100.0f);
    data.proj[1][1] *= -1;
    data.fov_y = fov;
    buffer.Update(g_ctx.vk, &data, sizeof(CameraData));
}

void Camera::update_aspect_ratio(const uint32_t width, const uint32_t height)
{
    data.proj = glm::perspective(glm::radians(data.fov_y), width / (float)height, 0.1f, 100.0f);
    data.proj[1][1] *= -1;
    data.aspect_ratio = width / (float)height;
    data.width = width;
    data.height = height;
    buffer.Update(g_ctx.vk, &data, sizeof(CameraData));
}

void Camera::update_rotation(const float dx, const float dy)
{
    auto right = glm::vec3(0, 0, 1);

    rotation -= 0.1f * glm::vec2(dx, dy);
    rotation.y = glm::clamp(rotation.y, -89.f, 89.f);
    if (rotation.x >= 360.f)
        rotation.x -= 360.f;
    if (rotation.x <= -360.f)
        rotation.x += 360.f;

    glm::quat rotate_x = glm::angleAxis(glm::radians(rotation.x), glm::vec3(0, 1, 0));
    glm::quat rotate_y = glm::angleAxis(glm::radians(rotation.y), right);
    glm::mat3x3 rotation_mat = glm::toMat3(glm::normalize(rotate_x * rotate_y));

    update_view_dir(glm::normalize(rotation_mat * init_view_dir));
}

void Camera::update_rotation(const glm::vec2& rotation)
{
    auto yaw = glm::radians(rotation.x);
    auto pitch = glm::radians(rotation.y);

    glm::quat yaw_quat = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
    glm::quat pitch_quat = glm::angleAxis(pitch, glm::vec3(0, 0, 1));

    glm::quat rotation_quat = yaw_quat * pitch_quat;
    glm::mat3x3 rotation_mat = glm::toMat3(rotation_quat);

    update_view_dir(glm::normalize(rotation_mat * init_view_dir));
}

void Camera::go_forward()
{
    data.eye_w += g_ctx.frame_time * move_speed * data.view_dir;
    update_position(data.eye_w);
}

void Camera::go_backward()
{
    data.eye_w -= g_ctx.frame_time * move_speed * data.view_dir;
    update_position(data.eye_w);
}

void Camera::go_left()
{
    auto right = glm::normalize(glm::cross(data.view_dir, glm::vec3(0, 1, 0)));
    data.eye_w -= g_ctx.frame_time * move_speed * right;
    update_position(data.eye_w);
}

void Camera::go_right()
{
    auto right = glm::normalize(glm::cross(data.view_dir, glm::vec3(0, 1, 0)));
    data.eye_w += g_ctx.frame_time * move_speed * right;
    update_position(data.eye_w);
}

void Camera::go_up()
{
    data.eye_w += g_ctx.frame_time * move_speed * glm::vec3(0, 1, 0);
    update_position(data.eye_w);
}

void Camera::go_down()
{
    data.eye_w -= g_ctx.frame_time * move_speed * glm::vec3(0, 1, 0);
    update_position(data.eye_w);
}

Camera Camera::fromConfiguration(CameraConfiguration& config)
{
    Camera camera;
    camera.data.eye_w = arrayToVec3(config.position);
    camera.data.view_dir = glm::normalize(arrayToVec3(config.view));
    camera.data.up = glm::vec3(0, 1, 0);
    camera.data.fov_y = config.fov;
    camera.data.width = g_ctx.vk.swapChainImages[0]->extent.width;
    camera.data.height = g_ctx.vk.swapChainImages[0]->extent.height;

    camera.data.view = glm::lookAt(camera.data.eye_w, camera.data.eye_w + camera.data.view_dir, camera.data.up);
    camera.data.aspect_ratio = camera.data.width / (float)camera.data.height;
    camera.data.proj = glm::perspective(glm::radians(camera.data.fov_y), camera.data.aspect_ratio, 0.1f, 100.0f);
    camera.data.proj[1][1] *= -1;
    camera.data.focal_distance = 0.1f;

    camera.move_speed = config.move_speed;
    camera.init_view_dir = glm::normalize(glm::vec3(1, 0, 0));

    float theta = glm::acos(glm::dot(camera.data.view_dir, glm::vec3(0, 1, 0)));
    assert(glm::abs(theta) >= 0.01f);
    theta = glm::pi<float>() / 2 - theta;
    glm::vec3 view_dir_xz(camera.data.view_dir.x, 0, camera.data.view_dir.z);
    view_dir_xz = glm::normalize(view_dir_xz);
    float phi = -glm::acos(glm::dot(view_dir_xz, glm::vec3(1, 0, 0)));
    if (glm::dot(view_dir_xz, glm::vec3(0, 0, 1)) < 0.f)
        phi = -phi;
    camera.rotation = glm::degrees(glm::vec2(phi, theta));

    camera.buffer = Vk::Buffer::New(
        g_ctx.vk,
        sizeof(CameraData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    camera.buffer.Update(g_ctx.vk, &camera.data, sizeof(CameraData));
    g_ctx.dm.registerResource(camera.buffer, DescriptorType::Uniform);

    return camera;
}
