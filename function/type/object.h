#pragma once

#include "core/config/config.h"
#include "core/vulkan/descriptor_manager.h"
#include "core/vulkan/type/buffer.h"
#include "function/resource_manager/resource.h"
#include "glm/glm.hpp"

#ifdef _WIN64
#include <Windows.h>
#endif

struct Object : public Resource {
    struct Param {
        Vk::DescriptorHandle material;
        glm::vec3 padding;
        glm::mat4 model;
        glm::mat4 modelInvTrans;
    };

    std::string name;
    uuid::UUID uuid;

    std::string mesh;
    glm::vec3 translate;
    glm::vec3 rotate;
    glm::vec3 scale;

    Param param;
    Vk::Buffer paramBuffer;

#ifdef _WIN64
    HANDLE getVkVertexMemHandle();
#else
    int getVkVertexMemHandle();
#endif
    virtual std::string type() const override { return "Object"; }
    void updateTransform(); // update param.model using translate, rotate, scale
    virtual void destroy() override;
    static Object fromConfiguration(ObjectConfiguration& config);
};
