#pragma once

#include "core/config/config.h"
#include "core/tool/recorder.h"
#include "function/resource_manager/resource.h"
#include "function/type/camera.h"
#include "function/type/field.h"
#include "function/type/light.h"
#include "function/type/material.h"
#include "function/type/mesh.h"
#include "function/type/object.h"
#include "function/type/texture.h"

class ResourceManager {
public:
    Camera camera;
    Lights lights;
    std::unordered_map<std::string, Mesh> meshes;
    std::unordered_map<std::string, Material> materials;
    std::unordered_map<std::string, Texture> textures;

    std::vector<Object> objects;
    Fields fields;

    Recorder recorder;

    std::unordered_map<std::string, std::unique_ptr<Resource>> resources;

    void load(Configuration& config);
    void addResource(std::unique_ptr<Resource> resource);
    void removeResource(const std::string& name);
    void cleanup();

private:
    void loadDefaultTextures();

    Configuration config;
};
