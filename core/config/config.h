#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

using MeshConfiguration = json;

struct TextureConfiguration {
    std::string name;
    std::string path;
};

struct MaterialConfiguration {
    std::string name;
    float roughness;
    float metallic;
    std::array<float, 3> color;
    std::string color_texture;
    std::string metallic_texture;
    std::string roughness_texture;
    std::string normal_texture;
    std::string ao_texture;
};

struct ObjectConfiguration {
    std::string name;
    std::string mesh;
    std::string material;
};

struct LightConfiguration {
    std::array<float, 3> posOrDir;
    std::array<float, 3> intensity;
};

struct CameraConfiguration {
    std::array<float, 3> position;
    std::array<float, 3> view;
    float fov;
    float move_speed;
};

struct RenderGraphConfiguration {
    std::string name;
    std::string shader_directory;
    json extra_args;
};

struct FieldConfiguration {
    std::string name;
    std::string path;
    std::string data_type;
    std::array<float, 3> start_pos;
    std::array<float, 3> size;
    std::array<int, 3> dimension;
    std::array<float, 3> scatter;
    std::array<float, 3> absorption;
};

struct FireConfiguration {
    std::array<int, 3> light_sample_dim;
    std::array<float, 3> light_sample_avg_region;
    std::vector<std::array<float, 3>> self_illumination_lights;
    float light_sample_gain;
    float self_illumination_boost;
    std::string fire_colors_path;
};

struct FieldsConfiguration {
    float step;
    json fire_configuration;
    std::vector<FieldConfiguration> arr;
};

struct RigidConfiguration {
    bool use_rigid;
};

struct EmitterConfiguration {
    bool use_emitter;
    std::array<int, 3> tile_dim;
    float dx;
    std::array<float, 3> grid_origin;
    std::string phi_path;
    float thickness;
    float temperature_coef;
    float buoyancy_coef;
};

struct RigidCoupleConfiguration {
    std::array<int, 3> tile_dim;
    float dx;
    std::array<float, 3> grid_origin;
    std::array<char, 3> neg_bc_type;
    std::array<char, 3> pos_bc_type;
    std::array<float, 3> neg_bc_val;
    std::array<float, 3> pos_bc_val;
    bool use_maccormac;
    bool poisson_is_uniform;
    RigidConfiguration rigid;
    EmitterConfiguration emitter;
};

struct DriverConfiguration {
    int total_frame;
    int frame_rate;
    int steps_per_frame;
};

struct LoggerConfiguration {
    std::string level;
    std::string output;
};

struct RecorderConfiguration {
    std::string output_path;
    int64_t bit_rate;
    int frame_rate;
    bool record_from_start;
};

struct RigidCoupleSimConfiguration {
    static RigidCoupleSimConfiguration load(const std::string& config_path);

    RigidCoupleConfiguration rigid_couple;
    DriverConfiguration driver;
    std::string output_dir;
};

using Configuration = json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    TextureConfiguration,
    name,
    path);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    MaterialConfiguration,
    name,
    roughness,
    metallic,
    color,
    color_texture,
    metallic_texture,
    roughness_texture,
    normal_texture,
    ao_texture);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    CameraConfiguration,
    position,
    view,
    fov,
    move_speed);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    RenderGraphConfiguration,
    name,
    shader_directory,
    extra_args);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    ObjectConfiguration,
    name,
    mesh,
    material);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    LightConfiguration,
    posOrDir,
    intensity);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    FieldConfiguration,
    name,
    path,
    data_type,
    start_pos,
    size,
    dimension,
    scatter,
    absorption);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    FireConfiguration,
    light_sample_dim,
    light_sample_avg_region,
    light_sample_gain,
    self_illumination_lights,
    self_illumination_boost,
    fire_colors_path);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    FieldsConfiguration,
    step,
    fire_configuration,
    arr);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    RigidConfiguration,
    use_rigid);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    EmitterConfiguration,
    use_emitter,
    tile_dim,
    dx,
    grid_origin,
    phi_path,
    thickness,
    temperature_coef,
    buoyancy_coef);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    RigidCoupleConfiguration,
    tile_dim,
    dx,
    grid_origin,
    neg_bc_type,
    pos_bc_type,
    neg_bc_val,
    pos_bc_val,
    use_maccormac,
    poisson_is_uniform,
    rigid,
    emitter);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    DriverConfiguration,
    total_frame,
    frame_rate,
    steps_per_frame);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    LoggerConfiguration,
    level,
    output);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    RecorderConfiguration,
    output_path,
    bit_rate,
    frame_rate,
    record_from_start);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    RigidCoupleSimConfiguration,
    rigid_couple,
    driver,
    output_dir);

#define JSON_GET(type, name, j, key)                                                                                                 \
    type name;                                                                                                                       \
    try {                                                                                                                            \
        auto&& temp = j[key];                                                                                                        \
        if (temp == nullptr) {                                                                                                       \
            throw std::runtime_error(std::string("Key '") + key + std::string("' not found"));                                       \
        }                                                                                                                            \
        name = std::move(temp.get<type>());                                                                                          \
    } catch (json::out_of_range & e) {                                                                                               \
        throw std::runtime_error(std::string("Can't cast JSON[\"") + key + std::string("\"] to type '") + std::string(#type) + "'"); \
    }

Configuration load(const std::string& config_path);
