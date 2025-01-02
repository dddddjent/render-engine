#pragma once

#include <string>

struct Resource {
    virtual ~Resource() = default;

    // for dynamic cast
    virtual std::string type() const { return "Resource"; }
    // destroy on resource manager cleanup
    virtual void destroy() = 0;

    std::string name;
};
