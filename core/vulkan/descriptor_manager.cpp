#include "descriptor_manager.h"
#include "core/vulkan/type/buffer.h"
#include "core/vulkan/vulkan_context.h"
#include <array>

namespace Vk {
void DescriptorManager::init(Context* ctx)
{
    this->ctx = ctx;

    initBindlessDescriptors();
    initBindlessTable();
    initParameters();
    initUI();
}

void DescriptorManager::initBindlessDescriptors()
{
    std::array<VkDescriptorSetLayoutBinding, TYPE_COUNT> bindings {};
    std::array<VkDescriptorBindingFlags, TYPE_COUNT> flags {};
    for (uint32_t i = 0; i < TYPE_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = types[i];
        bindings[i].descriptorCount = MAX_TYPE_DESCRIPTORS[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags {};
    bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlags.pBindingFlags = flags.data();
    bindingFlags.bindingCount = TYPE_COUNT;

    VkDescriptorSetLayoutCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = TYPE_COUNT;
    createInfo.pBindings = bindings.data();
    createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    createInfo.pNext = &bindingFlags;
    vkCreateDescriptorSetLayout(ctx->device, &createInfo, nullptr, &bindlessLayout);

    std::vector<VkDescriptorPoolSize> poolSize {};
    for (uint32_t i = 0; i < TYPE_COUNT; ++i) {
        poolSize.emplace_back(VkDescriptorPoolSize { types[i], static_cast<uint32_t>(MAX_TYPE_DESCRIPTORS[i]) });
    }
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes = poolSize.data();
    poolInfo.maxSets = 1;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    if (vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &bindlessPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = bindlessPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &bindlessLayout;
    if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &bindlessSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void DescriptorManager::initBindlessTable()
{
    for (uint32_t i = 0; i < TYPE_COUNT; ++i) {
        const auto& type = static_cast<DescriptorType>(i);
        allocatedHandles[type] = {};
        freeHandles[type].reserve(MAX_TYPE_DESCRIPTORS[i]);
        for (uint32_t j = 0; j < MAX_TYPE_DESCRIPTORS[i]; ++j)
            freeHandles[type].insert(static_cast<DescriptorHandle>(j));
    }
}

void DescriptorManager::initParameters()
{
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;
    if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, nullptr, &parameterLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = parameterPoolSize;

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = parameterPoolSize;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    if (vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &parameterPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorManager::initUI()
{
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 2;
    if (vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &uiPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

DescriptorHandle DescriptorManager::registerResource(const Image& image, DescriptorType type)
{
    assert(image.id != uuid::nil_uuid());
    assert(type == DescriptorType::CombinedImageSampler);
    if (freeHandles[type].empty()) {
        assert(allocatedHandles[type].size() == MAX_TYPE_DESCRIPTORS[static_cast<uint32_t>(type)]);
        throw std::runtime_error("failed to allocate descriptor handle!");
    }
    assert(allocatedHandles[type].size() < MAX_TYPE_DESCRIPTORS[static_cast<uint32_t>(type)]);

    const auto handle = *freeHandles[type].begin();
    freeHandles[type].erase(freeHandles[type].begin());
    allocatedHandles[type][image.id] = handle;
    nameToType[image.id] = type;

    assert(image.sampler != VK_NULL_HANDLE);
    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = image.layout;
    imageInfo.imageView = image.view;
    imageInfo.sampler = image.sampler;

    VkWriteDescriptorSet descriptorWrite {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = bindlessSet;
    descriptorWrite.dstBinding = static_cast<uint32_t>(type);
    descriptorWrite.dstArrayElement = static_cast<uint32_t>(handle);
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);

    return handle;
}

DescriptorHandle DescriptorManager::registerResource(const Buffer& buffer, DescriptorType type)
{
    assert(buffer.id != uuid::nil_uuid());
    assert(type == DescriptorType::Uniform || type == DescriptorType::Storage);
    if (freeHandles[type].empty()) {
        assert(allocatedHandles[type].size() == MAX_TYPE_DESCRIPTORS[static_cast<uint32_t>(type)]);
        throw std::runtime_error("failed to allocate descriptor handle!");
    }
    assert(allocatedHandles[type].size() < MAX_TYPE_DESCRIPTORS[static_cast<uint32_t>(type)]);

    const auto handle = *freeHandles[type].begin();
    freeHandles[type].erase(freeHandles[type].begin());
    allocatedHandles[type][buffer.id] = handle;
    nameToType[buffer.id] = type;

    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = bindlessSet;
    descriptorWrite.dstBinding = static_cast<uint32_t>(type);
    descriptorWrite.dstArrayElement = static_cast<uint32_t>(handle);
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    if (type == DescriptorType::Uniform)
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (type == DescriptorType::Storage)
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);

    return handle;
}

void DescriptorManager::updateResourceRegistration(const Image& image)
{
    assert(image.id != uuid::nil_uuid());
    const auto& type = nameToType.find(image.id);
    if (type == nameToType.end())
        throw std::runtime_error("failed to find the descriptor!");
    const auto& handle = allocatedHandles[type->second].find(image.id);
    if (handle == allocatedHandles[type->second].end())
        throw std::runtime_error("failed to find the descriptor!");

    assert(image.sampler != VK_NULL_HANDLE);
    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = image.layout;
    imageInfo.imageView = image.view;
    imageInfo.sampler = image.sampler;

    VkWriteDescriptorSet descriptorWrite {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = bindlessSet;
    descriptorWrite.dstBinding = static_cast<uint32_t>(type->second);
    descriptorWrite.dstArrayElement = static_cast<uint32_t>(handle->second);
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorManager::updateResourceRegistration(const Buffer& buffer)
{
    assert(buffer.id != uuid::nil_uuid());
    const auto& type = nameToType.find(buffer.id);
    if (type == nameToType.end())
        throw std::runtime_error("failed to find the descriptor!");
    const auto& handle = allocatedHandles[type->second].find(buffer.id);
    if (handle == allocatedHandles[type->second].end())
        throw std::runtime_error("failed to find the descriptor!");

    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = bindlessSet;
    descriptorWrite.dstBinding = static_cast<uint32_t>(type->second);
    descriptorWrite.dstArrayElement = static_cast<uint32_t>(handle->second);
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    if (type->second == DescriptorType::Uniform)
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (type->second == DescriptorType::Storage)
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);
}

DescriptorHandle DescriptorManager::getResourceHandle(const uuid::UUID& id)
{
    assert(id != uuid::nil_uuid());
    const auto& type = nameToType.find(id);
    if (type == nameToType.end())
        throw std::runtime_error("failed to find the descriptor!");
    const auto& handle = allocatedHandles[type->second].find(id);
    if (handle == allocatedHandles[type->second].end())
        throw std::runtime_error("failed to find the descriptor!");
    return handle->second;
}

void DescriptorManager::removeResourceRegistration(const uuid::UUID& id)
{
    assert(id != uuid::nil_uuid());
    const auto& type = nameToType.find(id);
    if (type == nameToType.end())
        throw std::runtime_error("failed to find the descriptor!");
    const auto& handle = allocatedHandles[type->second].find(id);
    if (handle == allocatedHandles[type->second].end())
        throw std::runtime_error("failed to find the descriptor!");
    freeHandles[type->second].emplace(handle->second);
    allocatedHandles[type->second].erase(id);
    nameToType.erase(id);
}

void DescriptorManager::resizeParameterPool()
{
    auto new_size = parameterPoolSize * 2;

    VkDescriptorPool new_pool;
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = new_size;
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = new_size;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    if (vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &new_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    std::vector<VkDescriptorSet> sets(new_size);
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = new_pool;
    allocInfo.descriptorSetCount = sets.size();
    allocInfo.pSetLayouts = &parameterLayout;
    if (vkAllocateDescriptorSets(ctx->device, &allocInfo, sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    std::vector<VkCopyDescriptorSet> copies(sets.size());
    int i = 0;
    for (auto& p : parameterSet) {
        copies[i].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
        copies[i].srcSet = p.second; // Source descriptor set
        copies[i].srcBinding = 0; // Source binding
        copies[i].srcArrayElement = 0; // Starting array index in source
        copies[i].dstSet = sets[i]; // Destination descriptor set
        copies[i].dstBinding = 0; // Destination binding
        copies[i].dstArrayElement = 0; // Starting array index in destination
        copies[i].descriptorCount = 1; // Number of descriptors to copy

        p.second = sets[i];
        i++;
    }
    vkUpdateDescriptorSets(ctx->device, 0, nullptr, copies.size(), copies.data());

    vkDestroyDescriptorPool(ctx->device, parameterPool, nullptr);
    parameterPool = new_pool;
    parameterPoolSize = new_size;
}

void DescriptorManager::registerParameter(const Buffer& buffer)
{
    assert(buffer.id != uuid::nil_uuid());
    if (parameterSet.find(buffer.id) != parameterSet.end())
        throw std::runtime_error("register the same uuid again!");

    if (parameterSet.size() == parameterPoolSize)
        resizeParameterPool();

    VkDescriptorSet desc_set;
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = parameterPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &parameterLayout;
    if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &desc_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = desc_set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);

    parameterSet[buffer.id] = desc_set;
}

VkDescriptorSet* DescriptorManager::getParameterSet(const uuid::UUID& uuid)
{
    assert(uuid != uuid::nil_uuid());
    const auto it = parameterSet.find(uuid);
    if (it == parameterSet.end())
        throw std::runtime_error("failed to find the descriptor set!");
    return &it->second;
}

void DescriptorManager::removeParameter(const uuid::UUID& uuid)
{
    assert(uuid != uuid::nil_uuid());
    const auto it = parameterSet.find(uuid);
    if (it == parameterSet.end())
        throw std::runtime_error("failed to find the descriptor set!");
    vkFreeDescriptorSets(ctx->device, parameterPool, 1, &it->second);
    parameterSet.erase(it);
}

void DescriptorManager::cleanup()
{
    vkDestroyDescriptorSetLayout(ctx->device, bindlessLayout, nullptr);
    vkDestroyDescriptorPool(ctx->device, bindlessPool, nullptr);

    vkDestroyDescriptorSetLayout(ctx->device, parameterLayout, nullptr);
    vkDestroyDescriptorPool(ctx->device, parameterPool, nullptr);

    vkDestroyDescriptorPool(ctx->device, uiPool, nullptr);
}
}
