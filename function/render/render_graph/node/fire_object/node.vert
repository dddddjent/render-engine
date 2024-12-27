#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

layout(set = 0, binding = BindlessUniformBinding) uniform Camera
{
    mat4 view;
    mat4 proj;
    vec3 eye_w;
    float fov;
    vec3 view_dir;
    float aspect_ratio;
    vec3 up;
    float focal_distance;
    float width;
    float height;
}
camera[];

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle camera;
}
pipelineParam;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 position_w;
layout(location = 1) out vec3 normal_w;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec3 tangent_w;

#define GetCamera camera[pipelineParam.camera]

void main()
{
    gl_Position = GetCamera.proj * GetCamera.view * vec4(inPosition, 1.0);
    position_w = inPosition;
    normal_w = inNormal;
    uv = inUV;
    tangent_w = inTangent;
}
