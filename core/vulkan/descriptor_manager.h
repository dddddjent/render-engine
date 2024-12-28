#pragma once

#include "core/vulkan/type/image.h"
#include <array>
#include <boost/uuid/uuid.hpp>
#include <core/tool/uuid.h>
#include <unordered_map>
#include <unordered_set>
#include <vulkan/vulkan.h>

namespace Vk {

struct Context;

enum class DescriptorHandle : uint32_t {
    Null = static_cast<uint32_t>(-1),
};

enum class DescriptorType : uint8_t {
    Uniform              = 0,
    Storage              = 1,
    CombinedImageSampler = 2,

    Count = 3,
};

class DescriptorManager {
    void initBindlessDescriptors();
    void initBindlessTable();
    void initParameters();
    void initUI();

    void resizeParameterPool();

    Context* ctx;

    VkDescriptorPool bindlessPool;
    VkDescriptorSetLayout bindlessLayout;
    VkDescriptorSet bindlessSet;
    std::unordered_map<DescriptorType, std::unordered_set<DescriptorHandle>> freeHandles;
    std::unordered_map<DescriptorType, std::unordered_map<uuid::UUID, DescriptorHandle>> allocatedHandles;
    std::unordered_map<uuid::UUID, DescriptorType> nameToType;

    VkDescriptorPool parameterPool;
    VkDescriptorSetLayout parameterLayout;
    std::unordered_map<uuid::UUID, VkDescriptorSet> parameterSet;
    uint32_t parameterPoolSize = 128;

public:
    DescriptorManager() = default;
    void init(Context* ctx);

    DescriptorHandle registerResource(const Image& image, DescriptorType type = DescriptorType::CombinedImageSampler);
    DescriptorHandle registerResource(const Buffer& buffer, DescriptorType type);
    void updateResourceRegistration(const Image& image);
    void updateResourceRegistration(const Buffer& buffer);
    DescriptorHandle getResourceHandle(const uuid::UUID& id);
    void removeResourceRegistration(const uuid::UUID& id);
    constexpr VkDescriptorSet* BINDLESS_SET() { return &bindlessSet; }
    constexpr VkDescriptorSetLayout BINDLESS_LAYOUT() { return bindlessLayout; }

    void registerParameter(const Buffer& buffer);
    void removeParameter(const uuid::UUID& uuid);
    VkDescriptorSet* getParameterSet(const uuid::UUID& uuid);
    constexpr VkDescriptorSetLayout PARAMETER_LAYOUT() const { return parameterLayout; }

    void cleanup();

    static constexpr size_t MAX_UNIFORM_DESCRIPTORS                = 1024;
    static constexpr size_t MAX_STORAGE_DESCRIPTORS                = 1024;
    static constexpr size_t MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTORS = 1024;
    static constexpr size_t TYPE_COUNT                             = static_cast<size_t>(DescriptorType::Count);
    static constexpr size_t MAX_TYPE_DESCRIPTORS[TYPE_COUNT]       = {
        MAX_UNIFORM_DESCRIPTORS,
        MAX_STORAGE_DESCRIPTORS,
        MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTORS,
    };
    static constexpr std::array<VkDescriptorType, TYPE_COUNT> types {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    };

    VkDescriptorPool uiPool;
};

extern DescriptorManager descriptor_manager;
}
