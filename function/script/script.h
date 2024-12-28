#pragma once

#include "core/config/config.h"
#include "function/global_context.h"

class Script {
public:
    // e.g. change config
    virtual void before_g_ctx(Configuration& config) { };
    // e.g. load custom resources
    virtual void before_internal_engine_init(Configuration& config) { };
    // regular init
    virtual void init(Configuration& config) { };

    virtual void step(float dt) { };
    virtual void destroy() { };
};
