#include "resource_manager.h"
#include "function/global_context.h"

using namespace Vk;

void ResourceManager::load(Configuration& config)
{
    camera = Camera::fromConfiguration(config.camera);
    lights = Lights::fromConfiguration(config.lights);

    for (auto& cfg : config.meshes) {
        auto mesh = Mesh::fromConfiguration(cfg);
        meshes[mesh.name] = mesh;
    }

    loadDefaultTextures();
    for (auto& cfg : config.textures) {
        auto texture = Texture::fromConfiguration(cfg);
        textures[texture.name] = texture;
    }

    for (auto& cfg : config.materials) {
        auto material = Material::fromConfiguration(cfg);
        materials[material.name] = material;
    }

    fields = Fields::fromConfiguration(config.fields);

    for (auto& cfg : config.objects) {
        objects.emplace_back(Object::fromConfiguration(cfg));
    }

    recorder.init(config);
}

void ResourceManager::cleanup()
{
    recorder.destroy();

    camera.destroy();
    lights.destroy();
    for (auto& mesh : meshes) {
        mesh.second.destroy();
    }
    for (auto& mat : materials) {
        mat.second.destroy();
    }
    for (auto& texture : textures) {
        texture.second.destroy();
    }
    for (auto& object : objects) {
        object.destroy();
    }
    fields.destroy();
}

void ResourceManager::loadDefaultTextures()
{
    textures["default_color"] = Texture::loadDefaultColorTexture();
    textures["default_metallic"] = Texture::loadDefaultMetallicTexture();
    textures["default_roughness"] = Texture::loadDefaultRoughnessTexture();
    textures["default_normal"] = Texture::loadDefaultNormalTexture();
    textures["default_ao"] = Texture::loadDefaultAoTexture();
}
