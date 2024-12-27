#pragma once

#include <glm/glm.hpp>

struct AABB {
    glm::vec3 bmin;
    float padding0;
    glm::vec3 bmax;
    float padding1;
};
