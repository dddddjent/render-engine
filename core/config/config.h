#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using PropertyVariant = std::variant<
    std::string,
    float,
    std::vector<float>,
    bool>;
// nlohmann::json>;

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

using MeshConfiguration = std::unordered_map<std::string, PropertyVariant>;

struct LightConfiguration {
    std::array<float, 3> posOrDir;
    std::array<float, 3> intensity;
};

using CameraConfiguration = std::unordered_map<std::string, PropertyVariant>;

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
    FireConfiguration fire_configuration;
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

struct Configuration {

    static Configuration load(const std::string& config_path);

    std::string name;
    std::string engine_directory;
    LoggerConfiguration logger;
    uint32_t width;
    uint32_t height;

    CameraConfiguration camera;
    std::string render_graph;

    std::vector<ObjectConfiguration> objects;
    FieldsConfiguration fields;

    std::string shader_directory;
    std::vector<LightConfiguration> lights;
    std::vector<MeshConfiguration> meshes;
    std::vector<MaterialConfiguration> materials;
    std::vector<TextureConfiguration> textures;

    RigidCoupleConfiguration rigid_couple;
    DriverConfiguration driver;

    RecorderConfiguration recorder;
};

struct RigidCoupleSimConfiguration {
    static RigidCoupleSimConfiguration load(const std::string& config_path);

    RigidCoupleConfiguration rigid_couple;
    DriverConfiguration driver;
    std::string output_dir;
};

#define JSON_GET(type, name, j, key)                                                                                                 \
    type name;                                                                                                                       \
    try {                                                                                                                            \
        auto&& temp = j[key];                                                                                                        \
        if (temp == nullptr) {                                                                                                       \
            throw std::runtime_error(std::string("Key '") + key + std::string("' not found"));                                       \
        }                                                                                                                            \
        name = std::move(temp);                                                                                                      \
    } catch (json::out_of_range & e) {                                                                                               \
        throw std::runtime_error(std::string("Can't cast JSON[\"") + key + std::string("\"] to type '") + std::string(#type) + "'"); \
    }
