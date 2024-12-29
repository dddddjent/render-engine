#include "geometry.h"
#include "glm/ext/matrix_transform.hpp"

std::pair<std::vector<Vertex>, std::vector<uint32_t>>
GeometryGenerator::sphere(glm::vec3 pos, float radius, int tessellation)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (unsigned int lat = 1; lat < tessellation; ++lat) {
        float theta = lat * M_PI / tessellation;
        for (unsigned int lon = 0; lon <= tessellation; ++lon) {
            float phi = lon * 2 * M_PI / tessellation;

            float x = radius * sin(theta) * cos(phi);
            float y = radius * cos(theta);
            float z = radius * sin(theta) * sin(phi);
            vertices.emplace_back();
            vertices.back().pos    = glm::vec3 { x, y, z } + pos;
            vertices.back().normal = glm::normalize(glm::vec3 { x, y, z });

            float u            = (float)lon / tessellation;
            float v            = (float)lat / tessellation;
            vertices.back().uv = glm::vec2(u, v);

            vertices.back().tangent = -glm::cross(vertices.back().normal, glm::normalize(glm::vec3 { cos(theta) * cos(phi), -sin(theta), cos(theta) * sin(phi) }));
        }
    }
    int top = vertices.size();
    for (int lon = 0; lon <= tessellation; ++lon) {
        vertices.emplace_back();
        vertices.back().pos    = pos + glm::vec3 { 0, radius, 0 };
        vertices.back().normal = { 0, 1, 0 };

        float u            = (float)(lon + 0.5f) / tessellation;
        vertices.back().uv = glm::vec2(u, 0.0f);

        float phi               = (lon + 0.5f) * 2 * M_PI / tessellation;
        float x                 = cos(phi);
        float z                 = sin(phi);
        vertices.back().tangent = -glm::cross(vertices.back().normal, glm::normalize(glm::vec3 { x, 0, z }));
    }
    for (int lon = 0; lon <= tessellation; ++lon) {
        vertices.emplace_back();
        vertices.back().pos    = pos - glm::vec3 { 0, radius, 0 };
        vertices.back().normal = { 0, -1, 0 };

        float u            = (float)(lon + 0.5f) / tessellation;
        vertices.back().uv = glm::vec2(u, 1.0f);

        float phi               = (lon + 0.5f) * 2 * M_PI / tessellation;
        float x                 = cos(phi);
        float z                 = sin(phi);
        vertices.back().tangent = -glm::cross(vertices.back().normal, glm::normalize(glm::vec3 { x, 0, z }));
    }

    for (unsigned int lat = 0; lat < tessellation - 2; ++lat) {
        for (unsigned int lon = 0; lon < tessellation; ++lon) {
            unsigned int first  = lat * (tessellation + 1) + lon;
            unsigned int second = lat * (tessellation + 1) + (lon + 1) % (tessellation + 1);
            unsigned int third  = first + (tessellation + 1);
            unsigned int fourth = second + (tessellation + 1);

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(third);

            indices.push_back(second);
            indices.push_back(fourth);
            indices.push_back(third);
        }
    }
    for (int i = 0; i < tessellation; ++i) {
        unsigned int first  = top + i;
        unsigned int second = i;
        unsigned int third  = (i + 1) % (tessellation + 1);
        indices.push_back(first);
        indices.push_back(third);
        indices.push_back(second);
    }
    for (int i = 0; i < tessellation; ++i) {
        unsigned int first  = top + (tessellation + 1) + i;
        unsigned int second = top - (tessellation + 1) + i;
        unsigned int third  = top - (tessellation + 1) + (i + 1) % (tessellation + 1);
        indices.push_back(first);
        indices.push_back(second);
        indices.push_back(third);
    }
    return { vertices, indices };
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>>
GeometryGenerator::cube(glm::vec3 pos, glm::vec3 scale)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::array<glm::vec3, 8> positions = { {
        { -0.5f, -0.5f, 0.5f },
        { 0.5f, -0.5f, 0.5f },
        { 0.5f, 0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f },
        { -0.5f, -0.5f, -0.5f },
        { 0.5f, -0.5f, -0.5f },
        { 0.5f, 0.5f, -0.5f },
        { -0.5f, 0.5f, -0.5f },
    } };
    std::array<glm::vec3, 6> normals   = { {
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, -1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },
    } };
    std::array<glm::vec2, 8> uvs       = { {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    } };
    vertices                           = {
        { positions[0], normals[0], uvs[0] },
        { positions[1], normals[0], uvs[1] },
        { positions[2], normals[0], uvs[2] },
        { positions[3], normals[0], uvs[3] },

        { positions[4], normals[1], uvs[0] },
        { positions[5], normals[1], uvs[1] },
        { positions[6], normals[1], uvs[2] },
        { positions[7], normals[1], uvs[3] },

        { positions[0], normals[2], uvs[0] },
        { positions[1], normals[2], uvs[1] },
        { positions[4], normals[2], uvs[3] },
        { positions[5], normals[2], uvs[2] },

        { positions[1], normals[3], uvs[0] },
        { positions[2], normals[3], uvs[1] },
        { positions[5], normals[3], uvs[3] },
        { positions[6], normals[3], uvs[2] },

        { positions[2], normals[4], uvs[0] },
        { positions[3], normals[4], uvs[1] },
        { positions[6], normals[4], uvs[3] },
        { positions[7], normals[4], uvs[2] },

        { positions[3], normals[5], uvs[0] },
        { positions[0], normals[5], uvs[1] },
        { positions[7], normals[5], uvs[3] },
        { positions[4], normals[5], uvs[2] },
    };
    indices = {
        0,
        2,
        3,
        0,
        1,
        2,

        4,
        6,
        5,
        4,
        7,
        6,

        8,
        10,
        9,
        9,
        10,
        11,

        12,
        14,
        13,
        13,
        14,
        15,

        16,
        18,
        17,
        17,
        18,
        19,

        20,
        22,
        21,
        21,
        22,
        23,
    };

    glm::mat4x4 transform = glm::identity<glm::mat4x4>();
    transform             = glm::translate(transform, pos);
    transform             = glm::scale(transform, scale);
    auto transform_n      = glm::transpose(glm::inverse(transform));
    for (auto& vertex : vertices) {
        auto pos   = transform * glm::vec4(vertex.pos, 1.0f);
        vertex.pos = glm::vec3(pos.x, pos.y, pos.z);

        auto normal   = transform_n * glm::vec4(vertex.normal, 1.0f);
        vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
        vertex.normal = glm::normalize(vertex.normal);
    }

    assert(vertices.size() < UINT16_MAX);

    return { vertices, indices };
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>>
GeometryGenerator::plane(glm::vec3 pos, glm::vec3 normal, std::vector<float> size)
{
    // if the normal is in the y positive direction, then x is u, z is v
    // in external app, horizontal is u, vertical is v
    // uv = (0, 0) is (-0.5, y, -0.5), at the top left corner of the picture
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::vec3 up(0, 0, 1);
    float dot_normal_up = glm::dot(normal, glm::vec3(0, 0, 1));
    if (1 - std::abs(dot_normal_up) < glm::epsilon<float>()) {
        up = glm::vec3(0, 1, 0);
    }

    glm::vec3 tangent   = glm::normalize(glm::cross(normal, up));
    glm::vec3 bitangent = glm::normalize(glm::cross(tangent, normal));
    vertices            = {
        { pos + -0.5f * size[0] * tangent + -0.5f * size[1] * bitangent, normal, { 0.0f, 0.0f } },
        { pos + 0.5f * size[0] * tangent + -0.5f * size[1] * bitangent, normal, { 1.0f, 0.0f } },
        { pos + 0.5f * size[0] * tangent + 0.5f * size[1] * bitangent, normal, { 1.0f, 1.0f } },
        { pos + -0.5f * size[0] * tangent + 0.5f * size[1] * bitangent, normal, { 0.0f, 1.0f } },
    };
    indices = {
        0, 2, 1,
        0, 3, 2
    };

    assert(vertices.size() < UINT16_MAX);

    return { vertices, indices };
}
