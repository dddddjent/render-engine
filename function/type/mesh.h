#pragma once

#include "core/config/config.h"
#include "core/vulkan/type/buffer.h"
#include "vertex.h"
#include <vector>

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct Mesh {
    std::string name;

    MeshData data;
    Vk::Buffer vertexBuffer;
    Vk::Buffer indexBuffer;

    static Mesh fromConfiguration(MeshConfiguration& config);
    void calculateTangents();
    void destroy();

private:
    static Mesh sphereMesh(MeshConfiguration& config);
    static Mesh cubeMesh(MeshConfiguration& config);
    static Mesh planeMesh(MeshConfiguration& config);
    static Mesh objMesh(MeshConfiguration& config);

    static glm::vec3 computeTangent(
        const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
        const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3);
    // an arbitrary tangent around normal
    static glm::vec3 computeFallbackTangent(const glm::vec3& normal);
    void initBuffersFromData();
};
