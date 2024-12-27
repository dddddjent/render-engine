#pragma once
#define _USE_MATH_DEFINES
#include "function/type/vertex.h"
#include <glm/glm.hpp>
#include <vector>

struct GeometryGenerator {
    static std::pair<std::vector<Vertex>, std::vector<uint32_t>>
    sphere(glm::vec3 pos, float radius, int tessellation);

    static std::pair<std::vector<Vertex>, std::vector<uint32_t>>
    cube(glm::vec3 pos, glm::vec3 scale);

    static std::pair<std::vector<Vertex>, std::vector<uint32_t>>
    plane(glm::vec3 pos, glm::vec3 normal, std::vector<float> size);
};
