#include "texture.h"
#include "core/tool/logger.h"
#include "function/global_context.h"
#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <filesystem>

using namespace Vk;

void Texture::destroy()
{
    Image::Delete(g_ctx.vk, image);
}

Texture Texture::fromConfiguration(const TextureConfiguration& config)
{
    Texture texture;
    texture.name = config.name;

    const auto extension = std::filesystem::path(config.path).extension().string();
    texture.image        = loadExternalImage(config.path);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);

    return texture;
}

Vk::Image Texture::loadExternalImage(const std::string& path)
{
    using namespace boost::gil;

    auto width  = 0;
    auto height = 0;
    VkFormat format {};
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

    any_image<
        gray8_image_t, gray16_image_t, gray32_image_t,
        rgb8_image_t, rgb16_image_t, rgb32_image_t,
        rgba8_image_t, rgba16_image_t, rgba32_image_t>
        boost_image;
    try {
        const auto extension = std::filesystem::path(path).extension().string();
        if (extension == ".tif" || extension == ".tiff") {
            read_image(path, boost_image, tiff_tag {});
        } else if (extension == ".png") {
            read_image(path, boost_image, png_tag {});
        } else if (extension == ".jpg" || extension == ".jpeg") {
            read_image(path, boost_image, jpeg_tag {});
        } else {
            throw std::runtime_error("Image type not supported");
        }
    } catch (const std::exception& e) {
        ERROR_ALL("Failed to load image: " + std::string(e.what()));
    }
    auto boost_image_view = view(boost_image);
    uint8_t* ptr          = nullptr;
    std::vector<uint8_t> extra_converted_buffer;
    boost::variant2::visit([&](auto&& view) {
        using ViewType = std::decay_t<decltype(view)>;
        width          = view.width();
        height         = view.height();
        ptr            = reinterpret_cast<uint8_t*>(interleaved_view_get_raw_data(view));

        if constexpr (std::is_same_v<ViewType, gray8_view_t>) {
            format = VK_FORMAT_R8_UNORM;
        } else if constexpr (std::is_same_v<ViewType, gray16_view_t>) {
            format = VK_FORMAT_R16_SFLOAT;
        } else if constexpr (std::is_same_v<ViewType, gray32_view_t>) {
            format = VK_FORMAT_R32_SFLOAT;
        } else if constexpr (std::is_same_v<ViewType, rgb8_view_t>) {
            format = VK_FORMAT_R8G8B8A8_UNORM; // vulkan doesn't support 3 bytes formats
            extra_converted_buffer.resize(width * height * 4);
            auto rgba_view = interleaved_view(width, height,
                                              reinterpret_cast<rgba8_pixel_t*>(extra_converted_buffer.data()),
                                              width * sizeof(rgba8_pixel_t));
            copy_and_convert_pixels(view, rgba_view);
            ptr = reinterpret_cast<uint8_t*>(interleaved_view_get_raw_data(rgba_view));
        } else if constexpr (std::is_same_v<ViewType, rgb16_view_t>) {
            format = VK_FORMAT_R16G16B16_SFLOAT;
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if constexpr (std::is_same_v<ViewType, rgb32_view_t>) {
            format = VK_FORMAT_R32G32B32_SFLOAT;
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if constexpr (std::is_same_v<ViewType, rgba8_view_t>) {
            format = VK_FORMAT_R8G8B8A8_UNORM;
        } else if constexpr (std::is_same_v<ViewType, rgba16_view_t>) {
            format = VK_FORMAT_R16G16B16A16_SFLOAT;
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if constexpr (std::is_same_v<ViewType, rgba32_view_t>) {
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            tiling = VK_IMAGE_TILING_LINEAR;
        } else {
            assert(false);
        }
    },
                           boost_image_view);

    const auto extent = VkExtent3D {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1
    };
    Image image;
    image = Image::New(
        g_ctx.vk,
        format,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        tiling);
    image.Update(g_ctx.vk, ptr);
    image.AddSampler(g_ctx.vk,
                     VK_FILTER_LINEAR,
                     std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return image;
}

Texture Texture::loadDefaultColorTexture()
{
    Texture texture;
    texture.name              = "default_color";
    std::vector<float> colors = {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    const auto extent = VkExtent3D { 1, 1, 1 };
    texture.image     = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_LINEAR);
    texture.image.Update(g_ctx.vk, colors.data());
    texture.image.AddSampler(g_ctx.vk,
                             VK_FILTER_LINEAR,
                             std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    texture.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);
    return texture;
}

Texture Texture::loadDefaultMetallicTexture()
{
    Texture texture;
    texture.name                = "default_metallic";
    std::vector<float> metallic = {
        1.0f
    };
    const auto extent = VkExtent3D { 1, 1, 1 };
    texture.image     = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_OPTIMAL);
    texture.image.Update(g_ctx.vk, metallic.data());
    texture.image.AddSampler(g_ctx.vk,
                             VK_FILTER_LINEAR,
                             std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    texture.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);
    return texture;
}

Texture Texture::loadDefaultRoughnessTexture()
{
    Texture texture;
    texture.name                 = "default_roughness";
    std::vector<float> roughness = {
        1.0f
    };
    const auto extent = VkExtent3D { 1, 1, 1 };
    texture.image     = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_OPTIMAL);
    texture.image.Update(g_ctx.vk, roughness.data());
    texture.image.AddSampler(g_ctx.vk,
                             VK_FILTER_LINEAR,
                             std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    texture.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);
    return texture;
}

Texture Texture::loadDefaultNormalTexture()
{
    Texture texture;
    texture.name              = "default_normal";
    std::vector<float> normal = {
        0.5f, 0.5f, 1.0f, 0.0f
    };
    const auto extent = VkExtent3D { 1, 1, 1 };
    texture.image     = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_OPTIMAL);
    texture.image.Update(g_ctx.vk, normal.data());
    texture.image.AddSampler(g_ctx.vk,
                             VK_FILTER_LINEAR,
                             std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    texture.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);
    return texture;
}

Texture Texture::loadDefaultAoTexture()
{
    Texture texture;
    texture.name          = "default_ao";
    std::vector<float> ao = {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    const auto extent = VkExtent3D { 1, 1, 1 };
    texture.image     = Image::New(
        g_ctx.vk,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        false,
        VK_IMAGE_TILING_OPTIMAL);
    texture.image.Update(g_ctx.vk, ao.data());
    texture.image.AddSampler(g_ctx.vk,
                             VK_FILTER_LINEAR,
                             std::vector<VkSamplerAddressMode>(3, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    texture.image.TransitionLayoutSingleTime(g_ctx.vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    g_ctx.dm.registerResource(texture.image, DescriptorType::CombinedImageSampler);
    return texture;
}
