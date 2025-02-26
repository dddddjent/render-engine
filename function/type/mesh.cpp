#include "mesh.h"
#include "core/math/math.h"
#include "core/tool/logger.h"
#include "function/global_context.h"
#include "function/tool/geometry.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/functional/hash.hpp>

using namespace Vk;

struct IndexHash {
    size_t operator()(const tinyobj::index_t& index) const
    {
        size_t seed = 0;
        boost::hash_combine(seed, ((uint64_t)index.vertex_index << 32) | index.normal_index);
        boost::hash_combine(seed, index.texcoord_index);
        return seed;
    }
};

struct IndexEqual {
    bool operator()(const tinyobj::index_t& i1, const tinyobj::index_t& i2) const
    {
        return (i1.vertex_index == i2.vertex_index && i1.normal_index == i2.normal_index && i1.texcoord_index == i2.texcoord_index);
    }
};

Mesh Mesh::fromConfiguration(MeshConfiguration& config)
{
    Mesh mesh;

    std::string type = config.at("type").get<std::string>();
    if (type == "sphere") {
        mesh = sphereMesh(config);
    } else if (type == "cube") {
        mesh = cubeMesh(config);
    } else if (type == "plane") {
        mesh = planeMesh(config);
    } else if (type == "file") {
        mesh = fileMesh(config);
    } else {
        throw std::runtime_error("Mesh type not supported");
    }

    mesh.name = config.at("name").get<std::string>();

    return mesh;
}

void Mesh::initBuffersFromData()
{
    vertexBuffer = Buffer::New(
        g_ctx.vk,
        sizeof(Vertex) * data.vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer.Update(g_ctx.vk, data.vertices.data(), vertexBuffer.size);

    indexBuffer = Buffer::New(
        g_ctx.vk,
        sizeof(uint32_t) * data.indices.size(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer.Update(g_ctx.vk, data.indices.data(), indexBuffer.size);
}

Mesh Mesh::fileMesh(MeshConfiguration& config)
{
    Mesh mesh;

    std::string inputfile = config.at("path").get<std::string>();

    auto flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenNormals;
    if (config["flip_uv"] == nullptr || config["flip_uv"].get<bool>()) {
        flags |= aiProcess_FlipUVs;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputfile, flags);
    if (!scene) {
        ERROR_ALL("Assimp: " + std::string(importer.GetErrorString()));
        throw std::runtime_error("Assimp: " + std::string(importer.GetErrorString()));
    }
    assert(scene->mNumMeshes == 1 && "Only one mesh in a file is supported");

    const aiMesh* ai_mesh = scene->mMeshes[0];
    assert(ai_mesh->GetNumUVChannels() == 1 && "Only one UV channel is supported");

    mesh.data.vertices.resize(ai_mesh->mNumVertices);
    for (uint32_t i = 0; i < ai_mesh->mNumVertices; i++) {
        mesh.data.vertices[i].pos = glm::vec3(
            ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z);
        mesh.data.vertices[i].normal = glm::vec3(
            ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z);
        mesh.data.vertices[i].tangent = glm::vec3(
            ai_mesh->mTangents[i].x, ai_mesh->mTangents[i].y, ai_mesh->mTangents[i].z);
        mesh.data.vertices[i].uv = glm::vec2(
            ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
    }

    mesh.data.indices.resize(ai_mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < ai_mesh->mNumFaces; i++) {
        assert(ai_mesh->mFaces[i].mNumIndices == 3 && "Only triangles are supported");
        for (uint32_t j = 0; j < 3; j++)
            mesh.data.indices[i * 3 + j] = ai_mesh->mFaces[i].mIndices[j];
    }

    mesh.initBuffersFromData();

    return mesh;
}

Mesh Mesh::sphereMesh(MeshConfiguration& config)
{
    Mesh mesh;
    glm::vec3 pos    = arrayToVec3(config.at("pos").get<std::vector<float>>());
    float radius     = config.at("radius").get<float>();
    int tessellation = config.at("tessellation").get<int>();

    auto [vertices, indices] = GeometryGenerator::sphere(pos, radius, tessellation);
    mesh.data.vertices       = std::move(vertices);
    mesh.data.indices        = std::move(indices);
    // mesh.calculateTangents();

    mesh.initBuffersFromData();

    return mesh;
}

Mesh Mesh::cubeMesh(MeshConfiguration& config)
{
    Mesh mesh;
    glm::vec3 pos   = arrayToVec3(config.at("pos").get<std::vector<float>>());
    glm::vec3 scale = arrayToVec3(config.at("scale").get<std::vector<float>>());

    auto [vertices, indices] = GeometryGenerator::cube(pos, scale);
    mesh.data.vertices       = std::move(vertices);
    mesh.data.indices        = std::move(indices);
    mesh.calculateTangents();

    mesh.initBuffersFromData();

    return mesh;
}

Mesh Mesh::planeMesh(MeshConfiguration& config)
{
    Mesh mesh;
    glm::vec3 pos           = arrayToVec3(config.at("pos").get<std::vector<float>>());
    glm::vec3 normal        = arrayToVec3(config.at("normal").get<std::vector<float>>());
    std::vector<float> size = config.at("size").get<std::vector<float>>();

    auto [vertices, indices] = GeometryGenerator::plane(pos, normal, size);
    mesh.data.vertices       = std::move(vertices);
    mesh.data.indices        = std::move(indices);
    mesh.calculateTangents();

    mesh.initBuffersFromData();

    return mesh;
}

Mesh Mesh::objMesh(MeshConfiguration& config)
{
    Mesh mesh;

    std::string inputfile = config.at("path").get<std::string>();
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        if (!reader.Error().empty())
            ERROR_ALL("TinyObjReader: " + reader.Error());
        exit(1);
    }
    if (!reader.Warning().empty()) {
        if (std::string_view(reader.Warning()).find("Material file") == std::string_view::npos)
            WARN_ALL("TinyObjReader: " + reader.Warning());
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    assert(shapes.size() == 1);
    const auto& shape = shapes[0];
    std::unordered_map<tinyobj::index_t, uint32_t, IndexHash, IndexEqual> index_map;
    size_t index_offset = 0;
    for (size_t face_id = 0; face_id < shape.mesh.num_face_vertices.size(); face_id++) {
        assert(shape.mesh.num_face_vertices[face_id] == 3);
        for (int i = 0; i < 3; i++) {
            tinyobj::index_t idx = shape.mesh.indices[index_offset + i];
            assert(idx.vertex_index >= 0);
            assert(idx.texcoord_index >= 0);
            if (index_map.count(idx) == 0) {
                index_map[idx] = mesh.data.vertices.size();
                mesh.data.vertices.emplace_back(Vertex {
                    .pos = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]),
                    .normal = glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]),
                    .uv = glm::vec2(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]) });
            }
            mesh.data.indices.emplace_back(index_map[idx]);
        }
        index_offset += 3;
    }
    mesh.calculateTangents();

    mesh.initBuffersFromData();

    return mesh;
}

glm::vec3 Mesh::computeTangent(
    const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
    const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
    glm::vec3 edge1    = p2 - p1;
    glm::vec3 edge2    = p3 - p1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    if ((deltaUV1.x == 0.0f && deltaUV2.x == 0.0f)
        || (deltaUV1.y == 0.0f && deltaUV2.y == 0.0f)) {
        auto t1 = glm::normalize(edge1);
        auto n  = glm::normalize(glm::cross(edge1, edge2));
        return glm::normalize(t1 - glm::dot(t1, n) * n);
    }

    float f           = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);

    return glm::normalize(tangent);
}

glm::vec3 Mesh::computeFallbackTangent(const glm::vec3& normal)
{
    glm::vec3 normalized = glm::normalize(normal);

    glm::vec3 other = (glm::abs(normalized.x) < glm::abs(normalized.z))
        ? glm::vec3(1.0f, 0.0f, 0.0f) // Prefer x-axis
        : glm::vec3(0.0f, 0.0f, 1.0f); // Prefer z-axis

    glm::vec3 tangent = glm::normalize(glm::cross(normalized, other));
    return tangent;
}

void Mesh::calculateTangents()
{
    for (size_t i = 0; i < data.indices.size(); i += 3) {
        auto& v0  = data.vertices[data.indices[i + 0]];
        auto& v1  = data.vertices[data.indices[i + 1]];
        auto& v2  = data.vertices[data.indices[i + 2]];
        auto& uv0 = v0.uv;
        auto& uv1 = v1.uv;
        auto& uv2 = v2.uv;

        auto t = computeTangent(v0.pos, v1.pos, v2.pos, uv0, uv1, uv2);
        if (glm::isnan(t).x == true or glm::isnan(t).y == true or glm::isnan(t).z == true) {
            auto normal = glm::normalize(glm::cross(
                v1.pos - v0.pos, v2.pos - v0.pos));
            t           = computeFallbackTangent(normal);
        }
        v0.tangent += t;
        v1.tangent += t;
        v2.tangent += t;
    }

    for (auto& vertex : data.vertices) {
        assert(glm::length(vertex.tangent) > 0.0f);
        vertex.tangent = glm::normalize(vertex.tangent
                                        - vertex.normal * glm::dot(vertex.normal, vertex.tangent));
    }
}

void Mesh::destroy()
{
    Buffer::Delete(g_ctx.vk, vertexBuffer);
    Buffer::Delete(g_ctx.vk, indexBuffer);
}
