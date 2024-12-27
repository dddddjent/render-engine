#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

inline glm::ivec3 arrayToVec3(const std::array<int, 3>& arr)
{
    return glm::ivec3(arr[0], arr[1], arr[2]);
}

inline glm::vec3 arrayToVec3(const std::array<float, 3>& arr)
{
    return glm::vec3(arr[0], arr[1], arr[2]);
}

inline glm::vec3 arrayToVec3(const std::vector<float>& arr)
{
    return glm::vec3(arr[0], arr[1], arr[2]);
}
