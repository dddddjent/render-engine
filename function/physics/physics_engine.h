#pragma once

#include "core/config/config.h"

struct GlobalContext;

class PhysicsEngine {
public:
    virtual void init(Configuration& config, GlobalContext* g_ctx) = 0;
    virtual void step()                                            = 0;
    virtual void sync()                                            = 0;
    virtual void cleanup()                                         = 0;
};
