#pragma once

#include "core/config/config.h"
#include "core/vulkan/type/buffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct CameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 eye_w;
    float fov_y;
    glm::vec3 view_dir;
    float aspect_ratio;
    glm::vec3 up;
    float focal_distance;
    float width;
    float height;
};

struct Camera {
    CameraData data;

    float move_speed;
    glm::vec2 rotation;
    glm::vec3 init_view_dir;

    Vk::Buffer buffer;

    void destroy();
    void update_position(const glm::vec3& position);
    void update_view_dir(const glm::vec3& view_dir);
    void update_up(const glm::vec3& up);
    void update_fov(const float fov);
    void update_aspect_ratio(const uint32_t width, const uint32_t height);
    void update_rotation(const float dx, const float dy);
    void update_rotation(const glm::vec2& rotation);

    void go_forward();
    void go_backward();
    void go_left();
    void go_right();
    void go_up();
    void go_down();
    static Camera fromConfiguration(CameraConfiguration& config);
};
