#include "global_context.h"
#include "function/resource_manager/resource_manager.h"

GlobalContext g_ctx;

GlobalContext::GlobalContext()  = default;
GlobalContext::~GlobalContext() = default;

void GlobalContext::init(Configuration& config, GLFWwindow* window)
{
    vk.init(config, window);
    dm.init(&vk);

    rm = std::make_unique<ResourceManager>();
    rm->load(config);
}

void GlobalContext::cleanup()
{
    rm->cleanup();
    dm.cleanup();
    vk.cleanup();
}
