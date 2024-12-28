#include "material.h"
#include "core/math/math.h"
#include "core/tool/logger.h"
#include "function/global_context.h"
#include "function/resource_manager/resource_manager.h"

using namespace Vk;

void Material::destroy()
{
    Buffer::Delete(g_ctx.vk, buffer);
}

void Material::update(const MaterialData& data)
{
    this->data = data;
    buffer.Update(g_ctx.vk, &this->data, sizeof(data));
}

Material Material::fromConfiguration(const MaterialConfiguration& config)
{
    Material material;

    material.name               = config.name;
    material.data.roughness     = config.roughness;
    material.data.metallic      = config.metallic;
    material.data.color         = arrayToVec3(config.color);
    material.data.color_texture = g_ctx.dm.getResourceHandle(
        g_ctx.rm->textures[config.color_texture].image.id);
    material.data.metallic_texture = g_ctx.dm.getResourceHandle(
        g_ctx.rm->textures[config.metallic_texture].image.id);
    material.data.roughness_texture = g_ctx.dm.getResourceHandle(
        g_ctx.rm->textures[config.roughness_texture].image.id);
    material.data.normal_texture = g_ctx.dm.getResourceHandle(
        g_ctx.rm->textures[config.normal_texture].image.id);
    material.data.ao_texture = g_ctx.dm.getResourceHandle(
        g_ctx.rm->textures[config.ao_texture].image.id);

    material.buffer = Buffer::New(
        g_ctx.vk,
        sizeof(material.data),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    material.buffer.Update(g_ctx.vk, &material.data, sizeof(material.data));
    g_ctx.dm.registerResource(material.buffer, DescriptorType::Uniform);

    return material;
}
