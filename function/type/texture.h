#pragma once

#include "core/config/config.h"
#include "core/vulkan/type/image.h"
#include <string>

struct Texture {
    std::string name;

    Vk::Image image;

    void destroy();
    static Texture fromConfiguration(const TextureConfiguration& config);

    static Texture loadDefaultColorTexture();
    static Texture loadDefaultMetallicTexture();
    static Texture loadDefaultRoughnessTexture();
    static Texture loadDefaultNormalTexture();
    static Texture loadDefaultAoTexture();

private:
    static Vk::Image loadExternalImage(const std::string& path);
};
