#include "config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace nlohmann {
template <>
struct adl_serializer<PropertyVariant> {
    static void to_json(json& j, const PropertyVariant& v)
    {
        switch (v.index()) {
        case 0:
            j = std::get<std::string>(v);
            break;
        case 1:
            j = std::get<float>(v);
            break;
        case 2:
            j = std::get<std::vector<float>>(v);
            break;
        case 3:
            j = std::get<bool>(v);
            break;
        default:
            // j = std::get<json>(v);
            throw std::runtime_error("Unsupported type");
        }
    }

    static void from_json(const json& j, PropertyVariant& v)
    {
        if (j.is_string()) {
            v = j.get<std::string>();
        } else if (j.is_number()) {
            v = j.get<float>();
        } else if (j.is_array()) {
            v = j.get<std::vector<float>>();
        } else if (j.is_boolean()) {
            v = j.get<bool>();
        } else {
            // v = j.get<json>();
            throw std::runtime_error("Unsupported type");
        }
    }
};
}

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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
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
    Configuration,
    name,
    engine_directory,
    logger,
    width,
    height,
    camera,
    render_graph,
    objects,
    fields,
    lights,
    shader_directory,
    meshes,
    materials,
    textures,
    rigid_couple,
    driver,
    recorder);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    RigidCoupleSimConfiguration,
    rigid_couple,
    driver,
    output_dir);

Configuration Configuration::load(const std::string& config_path)
{
    std::ifstream f(config_path);

    json j;
    try {
        j = json::parse(f);
    } catch (json::parse_error& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    JSON_GET(ObjectConfiguration, o, j, "recorder");
    Configuration config = json::parse(f);
    f.seekg(0);

    return config;
}

RigidCoupleSimConfiguration RigidCoupleSimConfiguration::load(const std::string& config_path)
{
    std::ifstream f(config_path);
    RigidCoupleSimConfiguration config = json::parse(f);
    return config;
}
