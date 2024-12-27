#version 450

#define FIELD_COUNT 2
#define MAX_FIELDS 2
#define FIRE_SELF_ILLUMINATION_BOOST 20.0

#extension GL_GOOGLE_include_directive : enable

#include "../../shader/common.glsl"

const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const int MAX_LIGHTS = 16;
const float MAX = 100000000;
const float MIN = -100000000;

const int TYPE_CONCENTRATION = 0;
const int TYPE_TEMPERATURE = 1;

struct Light {
    vec3 posOrDir;
    vec3 intensity;
};

struct AABB {
    vec3 bmin;
    vec3 bmax;
};

struct FieldData {
    mat4x4 to_local_uvw;
    vec3 scatter;
    int type;
    vec3 absorption;
    AABB aabb;
};

layout(push_constant) uniform PushConstants
{
    float in_step;
};

layout(set = BindlessDescriptorSet, binding = BindlessUniformBinding) uniform Camera
{
    mat4 view;
    mat4 proj;
    vec3 eye_w;
    float fov;
    vec3 view_dir;
    float aspect_ratio;
    vec3 up;
    float focal_distance;
    int width;
    int height;
}
GetLayoutVariableName(camera)[];

layout(set = BindlessDescriptorSet, binding = BindlessUniformBinding) uniform FieldDataArray
{
    FieldData data;
}
GetLayoutVariableName(field_data_arr)[];

layout(set = BindlessDescriptorSet, binding = BindlessStorageBinding) readonly buffer Lights
{
    Light data[];
}
GetLayoutVariableName(lights)[];

layout(set = BindlessDescriptorSet, binding = BindlessStorageBinding) readonly buffer SelfIlluminationLights
{
    vec3 positions[];
}
GetLayoutVariableName(self_illumination_light)[];

layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler3D GetLayoutVariableName(field_image_sampler)[];
layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler2D GetLayoutVariableName(previous_color)[];
layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler2D GetLayoutVariableName(previous_depth)[];
layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding)
    uniform sampler2D GetLayoutVariableName(fire_color_sampler)[];

layout(set = 1, binding = 0) uniform PipelineParam
{
    Handle camera;
    Handle lights;
    Handle self_illumination_lights;
    Handle fire_color;
    Handle previous_color;
    Handle previous_depth;
}
pipelineParam;

layout(set = 2, binding = 0) uniform FieldParam
{
    Handle attr[MAX_FIELDS];
    Handle img[MAX_FIELDS];
}
fieldParam;

layout(location = 0) out vec4 outColor;

#define camera GetResource(camera, pipelineParam.camera)
#define lights GetResource(lights, pipelineParam.lights)
#define self_illumination_light GetResource(self_illumination_light, pipelineParam.self_illumination_lights)
#define field_data_arr(index) GetResource(field_data_arr, fieldParam.attr[index])
#define field_image_sampler(index) GetResource(field_image_sampler, fieldParam.img[index])
#define fire_color_sampler GetResource(fire_color_sampler, pipelineParam.fire_color)
#define previous_color GetResource(previous_color, pipelineParam.previous_color)
#define previous_depth GetResource(previous_depth, pipelineParam.previous_depth)

float random(float x)
{
    float y = fract(sin(x) * 100000.0);
    return y;
}

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

bool intersect_aabb(vec3 origin, vec3 dir, in AABB aabb, out float tentry, out float texit)
{
    vec3 t_min = (aabb.bmin - origin) / (dir + EPSILON);
    vec3 t_max = (aabb.bmax - origin) / (dir + EPSILON);
    vec3 t_entry = min(t_min, t_max);
    vec3 t_exit = max(t_min, t_max);
    tentry = max(t_entry.x, max(t_entry.y, t_entry.z));
    texit = min(t_exit.x, min(t_exit.y, t_exit.z));
    return tentry <= texit && texit >= 0;
}

float map_density(float sampled_density, int type)
{
    // some fields may be sampled outside of the field
    if (type == TYPE_TEMPERATURE) {
        return sampled_density * 20;
    }
    if (type == TYPE_CONCENTRATION) {
        return sampled_density * 180;
    }
    return 0;
}

float phase(const float g, const float cos_theta)
{
    float denom = 1 + g * g - 2 * g * cos_theta;
    return 1 / (4 * PI) * (1 - g * g) / (denom * sqrt(denom));
}

vec3 compute_light_in_scatter_multi(vec3 origin, vec3 ray_eye, Light light)
{
    vec3 ray = light.posOrDir - origin;
    float ray_length = length(ray);
    ray /= ray_length;
    float step = in_step * 4;

    float t_entry = MAX, t_exit = MIN;
    for (int i = 0; i < FIELD_COUNT; i++) {
        float t_entry_i = 0.0, t_exit_i = 0.0;

        intersect_aabb(origin, ray, field_data_arr(i).data.aabb, t_entry_i, t_exit_i);
        t_entry = min(t_entry, t_entry_i);
        t_exit = max(t_exit, t_exit_i);
    }

    vec3 local_rays[MAX_FIELDS];
    vec3 local_origins[MAX_FIELDS];
    for (int i = 0; i < FIELD_COUNT; i++) {
        local_rays[i] = (field_data_arr(i).data.to_local_uvw * vec4(ray, 0.0)).xyz;
        local_origins[i] = (field_data_arr(i).data.to_local_uvw * vec4(origin, 1.0)).xyz;
    }
    vec3 sigma_ts[MAX_FIELDS];
    for (int i = 0; i < FIELD_COUNT; i++) {
        sigma_ts[i] = field_data_arr(i).data.scatter + field_data_arr(i).data.absorption;
    }

    float t = max(t_entry, 0.0);
    vec3 transmittance = vec3(1.0f);
    vec3 sigma_t_density_sum = vec3(0.0);
    float densities[MAX_FIELDS];
    vec3 sample_point = vec3(0.0);
    while (true) {
        float t_sample = t + random(t) * step;
        if (t > t_exit)
            break;

        for (int i = 0; i < FIELD_COUNT; i++) {
            sample_point = local_origins[i] + local_rays[i] * t_sample;
            densities[i] = textureLod(field_image_sampler(i), sample_point, 0.0).r;
            densities[i] = map_density(densities[i], field_data_arr(i).data.type);
        }
        for (int i = 0; i < FIELD_COUNT; i++) {
            sigma_t_density_sum += densities[i] * sigma_ts[i];
        }

        t += step;
    }

    transmittance *= exp(-sigma_t_density_sum * step);
    float dot_ray_light = dot(-ray_eye, ray);
    return light.intensity * transmittance * phase(0.0, dot_ray_light) / (ray_length * ray_length);
}

vec3 emit_color(float temperature)
{
    return texture(fire_color_sampler, vec2(temperature, 0.0)).rgb;
}

vec3 compute_fire_in_scatter_multi(vec3 origin, vec3 ray_eye, vec3 sample_fire_pos)
{
    vec3 ray = normalize(sample_fire_pos - origin);
    float step = in_step * 4;

    float t_entry = MAX, t_exit = MIN;
    for (int i = 0; i < FIELD_COUNT; i++) {
        float t_entry_i = 0.0, t_exit_i = 0.0;

        intersect_aabb(origin, ray, field_data_arr(i).data.aabb, t_entry_i, t_exit_i);
        t_entry = min(t_entry, t_entry_i);
        t_exit = max(t_exit, t_exit_i);
    }
    // terminate at the fire's center
    t_exit = min(t_exit, length(sample_fire_pos - origin));

    vec3 local_rays[MAX_FIELDS];
    vec3 local_origins[MAX_FIELDS];
    for (int i = 0; i < FIELD_COUNT; i++) {
        local_rays[i] = (field_data_arr(i).data.to_local_uvw * vec4(ray, 0.0)).xyz;
        local_origins[i] = (field_data_arr(i).data.to_local_uvw * vec4(origin, 1.0)).xyz;
    }
    vec3 sigma_ts[MAX_FIELDS];
    for (int i = 0; i < FIELD_COUNT; i++) {
        sigma_ts[i] = field_data_arr(i).data.scatter + field_data_arr(i).data.absorption;
    }

    float t = max(t_entry, 0.0);
    vec3 sample_point = vec3(0.0);
    vec3 transmittance = vec3(1.0f);
    vec3 color = vec3(0.0f);
    while (true) {
        float t_sample = random(t) * step + t;
        if (t > t_exit)
            break;

        vec3 sigma_t_density_sum = vec3(0.0);
        for (int i = 0; i < FIELD_COUNT; i++) {
            sample_point = local_rays[i] * t_sample + local_origins[i];
            float density = textureLod(field_image_sampler(i), sample_point, 0.0).r;
            float mapped_density = map_density(density, field_data_arr(i).data.type);
            sigma_t_density_sum += mapped_density * (field_data_arr(i).data.scatter + field_data_arr(i).data.absorption);

            color += field_data_arr(i).data.type * transmittance * emit_color(density) * step;
        }

        transmittance *= exp(-sigma_t_density_sum * step);

        t += step;
    }

    return FIRE_SELF_ILLUMINATION_BOOST * color * phase(0.0, dot(-ray_eye, ray));
}

vec4 volumetric_color_multi(vec3 origin, vec3 ray, float depth, mat4x4 proj_view)
{
    vec4 object_color = texelFetch(previous_color, ivec2(gl_FragCoord.xy), 0);
    float step = in_step;
    int self_illumination_light_count = self_illumination_light.positions.length();

    float t_entry = MAX, t_exit = MIN;
    bool has_intersection = false;
    for (int i = 0; i < FIELD_COUNT; i++) {
        float t_entry_i = 0.0, t_exit_i = 0.0;

        bool result = intersect_aabb(origin, ray, field_data_arr(i).data.aabb, t_entry_i, t_exit_i);
        has_intersection = has_intersection || result;
        t_entry = min(t_entry, t_entry_i);
        t_exit = max(t_exit, t_exit_i);
    }
    if (!has_intersection)
        return object_color;

    vec4 clip_ray = proj_view * vec4(ray, 0.0);
    vec4 clip_origin = proj_view * vec4(origin + camera.focal_distance * ray, 1.0);
    vec3 local_rays[MAX_FIELDS];
    vec3 local_origins[MAX_FIELDS];
    for (int i = 0; i < FIELD_COUNT; i++) {
        local_rays[i] = (field_data_arr(i).data.to_local_uvw * vec4(ray, 0.0)).xyz;
        local_origins[i] = (field_data_arr(i).data.to_local_uvw * vec4(origin, 1.0)).xyz;
    }

    float t = max(t_entry, 0.0);
    vec3 transmittance = vec3(1.0f);
    vec3 color = vec3(0.0f);
    while (true) {
        float t_sample = t + random(t) * step;
        vec4 clip_point = clip_origin + (t_sample - camera.focal_distance) * clip_ray;
        if (t > t_exit || clip_point.z / clip_point.w > depth)
            break;

        vec3 sigma_t_density_sum = vec3(0.0);
        vec3 sigma_s_density_sum = vec3(0.0);

        for (int i = 0; i < FIELD_COUNT; i++) {
            vec3 sample_point = local_origins[i] + local_rays[i] * t_sample;
            float density = textureLod(field_image_sampler(i), sample_point, 0.0).r;
            float mapped_density = map_density(density, field_data_arr(i).data.type);
            sigma_t_density_sum += mapped_density * (field_data_arr(i).data.scatter + field_data_arr(i).data.absorption);

            if (field_data_arr(i).data.type == TYPE_CONCENTRATION)
                sigma_s_density_sum += mapped_density * field_data_arr(i).data.scatter;
            if (field_data_arr(i).data.type == TYPE_TEMPERATURE)
                color += transmittance * emit_color(density) * step;
        }

        if (length(sigma_s_density_sum) > 1e-3) {
            vec3 light_in_scatter = vec3(0.0);
            for (int i = 0; i < lights.data.length(); i++) {
                light_in_scatter += compute_light_in_scatter_multi(origin + t_sample * ray, ray, lights.data[i]);
            }
            for (int i = 0; i < self_illumination_light_count; i++) {
                light_in_scatter += compute_fire_in_scatter_multi(origin + t_sample * ray, ray, self_illumination_light.positions[i]);
            }
            color += transmittance * sigma_s_density_sum * light_in_scatter * step;
        }

        transmittance *= exp(-sigma_t_density_sum * step);

        t += step;
    }

    return vec4(color + transmittance * object_color.rgb, object_color.a);
}

void main()
{
    vec2 coord = gl_FragCoord.xy;
    coord = coord / vec2(camera.width, camera.height) - vec2(0.5);

    vec3 focal = camera.eye_w + camera.focal_distance * normalize(camera.view_dir);
    float height = 2 * tan(radians(camera.fov / 2)) * camera.focal_distance;
    float width = camera.aspect_ratio * height;
    vec3 right = normalize(cross(camera.view_dir, camera.up));
    vec3 down = normalize(cross(camera.view_dir, right));
    vec3 point = focal + coord.x * width * right + coord.y * height * down;
    vec3 ray = normalize(point - camera.eye_w);

    float depth = texelFetch(previous_depth, ivec2(gl_FragCoord.xy), 0).r;
    mat4x4 proj_view = camera.proj * camera.view;
    outColor = volumetric_color_multi(camera.eye_w, ray, depth, proj_view);
}
