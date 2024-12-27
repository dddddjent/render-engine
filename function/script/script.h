#pragma once

class Script {
public:
    virtual void init() {};
    virtual void step(float dt) {};
    virtual void destroy() {};
};
