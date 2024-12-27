#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf : enable

#include "../../shader/common.glsl"

struct Light {
    vec3 posOrDir;
    vec3 intensity;
};

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
GetLayoutVariableName(camera)[];

layout(set = 0, binding = BindlessUniformBinding) uniform Material
{
    vec3 color;
    float roughness;
    float metallic;
    Handle color_texture;
    Handle metallic_texture;
    Handle roughness_texture;
    Handle normal_texture;
    Handle ao_texture;
}
GetLayoutVariableName(material)[];

layout(set = 0, binding = BindlessStorageBinding) readonly buffer Lights
{
    Light data[];
}
GetLayoutVariableName(lights)[];

layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler2D GetLayoutVariableName(textures)[];

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle camera;
    Handle lights;
}
pipelineParam;

layout(set = 2, binding = 0) uniform ObjectParam
{
    Handle material;
}
objectParam;

#define camera GetResource(camera, pipelineParam.camera)
#define lights GetResource(lights, pipelineParam.lights)
#define MATERIAL GetResource(material, objectParam.material)
#define COLOR_TEXTURE GetResource(textures, MATERIAL.color_texture)
#define METALLIC_TEXTURE GetResource(textures, MATERIAL.metallic_texture)
#define ROUGHNESS_TEXTURE GetResource(textures, MATERIAL.roughness_texture)
#define NORMAL_TEXTURE GetResource(textures, MATERIAL.normal_texture)
#define AO_TEXTURE GetResource(textures, MATERIAL.ao_texture)

layout(location = 0) in vec3 position_w;
layout(location = 1) in vec3 normal_w;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent_w;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float D(vec3 h, vec3 normal, float roughness)
{
    float numerator = roughness * roughness;
    float dot_h_n = max(dot(normalize(normal), h), 0.0);
    float denominator = dot_h_n * dot_h_n * (roughness * roughness - 1) + 1;
    denominator = PI * denominator * denominator;
    return numerator / denominator;
}

vec3 F(vec3 F0, vec3 h, vec3 w_e)
{
    return F0 + (1 - F0) * pow(1 - max(dot(w_e, h), 0.0), 5);
}

float G(vec3 w_e, vec3 w_i, vec3 normal, float k)
{
    float dot_w_e_n = dot(w_e, normalize(normal));
    float g1 = dot_w_e_n / (dot_w_e_n * (1 - k) + k);

    float dot_w_i_n = dot(w_i, normalize(normal));
    float g2 = dot_w_i_n / (dot_w_i_n * (1 - k) + k);
    return g1 * g2;
}

vec3 mappedNormal(vec3 normal_w)
{
    vec3 sampled_normal = texture(NORMAL_TEXTURE, uv).rgb;
    vec3 normal_t = 2 * sampled_normal - vec3(1.0f);

    vec3 normal = normalize(normal_w);
    vec3 tangent = normalize(tangent_w - dot(tangent_w, normal) * normal);
    vec3 bitangent = cross(normal, tangent);

    mat3 tbn = mat3(tangent, -bitangent, normal);
    vec3 a = tbn * normal_t;

    return normalize(tbn * normal_t);
}

vec3 colorOnSingleLight(Light light)
{
    vec3 color = srgbToLinear(texture(COLOR_TEXTURE, uv).rgb * MATERIAL.color);
    float roughness = texture(ROUGHNESS_TEXTURE, uv).r * MATERIAL.roughness;
    float metallic = texture(METALLIC_TEXTURE, uv).r * MATERIAL.metallic;
    vec3 normal = mappedNormal(normal_w);

    vec3 w_e = normalize(camera.eye_w - position_w);
    vec3 w_i = normalize(light.posOrDir - position_w);

    float dist = length(light.posOrDir - position_w);
    vec3 intensity = light.intensity / (dist * dist);

    float dot_w_i_n = dot(normal, w_i);
    float dot_w_e_n = dot(normal, w_e);
    if (dot_w_e_n < 0 || dot_w_i_n < 0)
        return vec3(0);

    vec3 h = normalize(w_i + w_e);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, color, metallic);
    float k = (roughness + 1);
    k = k * k / 8;

    float D = D(h, normal, roughness);
    vec3 F = F(F0, h, w_e);
    float G = G(w_e, w_i, normal, k);
    float denominator = 4 * dot(w_e, normal) * dot_w_i_n;

    vec3 specular = intensity * D * F * G / denominator * dot_w_i_n;
    vec3 diffuse = intensity * color * dot_w_i_n / PI;

    vec3 kd = (1 - F) * (1 - metallic);
    return kd * diffuse + specular;
}

void main()
{
    vec3 color = vec3(0.0f);
    int lightCount = lights.data.length();
    for (int i = 0; i < lightCount; i++) {
        color += colorOnSingleLight(lights.data[i]);
    }

    vec3 ambient = srgbToLinear(texture(COLOR_TEXTURE, uv).rgb * MATERIAL.color);
    ambient *= texture(AO_TEXTURE, uv).rgb;
    color += vec3(0.03) * ambient;

    outColor = vec4(color, 1.0);
}
