#include "light.h"
#include "core/tool/logger.h"
#include "core/math/math.h"
#include "function/global_context.h"

using namespace Vk;

Lights Lights::fromConfiguration(const std::vector<LightConfiguration>& config)
{
    Lights lights;
    lights.name = "lights";

    for (int i = 0; i < config.size(); i++) {
        LightData light;
        light.posOrDir = arrayToVec3(config[i].posOrDir);
        light.intensity = arrayToVec3(config[i].intensity);
        lights.data.emplace_back(light);
    }
    lights.buffer = Buffer::New(
        g_ctx.vk,
        config.size() * sizeof(LightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);
    lights.buffer.Update(g_ctx.vk, lights.data.data(), lights.buffer.size);
    g_ctx.dm.registerResource(lights.buffer, DescriptorType::Storage);
    return lights;
}

void Lights::update(const LightData* data, int index, int cnt)
{
    for (int i = index; i < index + cnt; i++) {
        this->data[i] = data[i - index];
    }
    buffer.Update(g_ctx.vk, this->data.data() + index, cnt * sizeof(LightData), index * sizeof(LightData));
}

void Lights::destroy()
{
    buffer.Delete(g_ctx.vk, buffer);
}
