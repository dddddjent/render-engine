#pragma once

#include "core/vulkan/type/buffer.h"
#include "core/vulkan/vulkan_util.h"
#include "function/global_context.h"
#include "function/type/vertex.h"
#include <vulkan/vulkan_core.h>

#define VertexInputDefault(hasVertexInput)                                                                         \
        VkPipelineVertexInputStateCreateInfo vertexInput {};                                                   \
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;                         \
        auto bindingDescription = Vertex::getBindingDescription();                                                 \
        auto attributeDescriptions = Vertex::getAttributeDescriptions();                                           \
        if (hasVertexInput) {                                                                                      \
            vertexInput.vertexBindingDescriptionCount = 1;                                                     \
            vertexInput.pVertexBindingDescriptions = &bindingDescription;                                      \
            vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); \
            vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();                           \
        } else {                                                                                               \
            vertexInput.vertexBindingDescriptionCount = 0;                                                     \
            vertexInput.pVertexBindingDescriptions = nullptr;                                                  \
            vertexInput.vertexAttributeDescriptionCount = 0;                                                   \
            vertexInput.pVertexAttributeDescriptions = nullptr;                                                \
        }                                                                                                          \

#define DynamicStateDefault()                                                         \
        std::vector<VkDynamicState> dynamicStates = {                                 \
            VK_DYNAMIC_STATE_VIEWPORT,                                                \
            VK_DYNAMIC_STATE_SCISSOR                                                  \
        };                                                                            \
        VkPipelineDynamicStateCreateInfo dynamicState {};                             \
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;    \
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()); \
        dynamicState.pDynamicStates = dynamicStates.data();                           \

#define ViewportStateDefault()                                                       \
        VkViewport viewport {};                                                      \
        viewport.x = 0.0f;                                                           \
        viewport.y = 0.0f;                                                           \
        viewport.width = (float)g_ctx.vk.swapChainImages[0]->extent.width;               \
        viewport.height = (float)g_ctx.vk.swapChainImages[0]->extent.height;             \
        viewport.minDepth = 0.0f;                                                    \
        viewport.maxDepth = 1.0f;                                                    \
        VkRect2D scissor {};                                                         \
        scissor.offset = { 0, 0 };                                                   \
        scissor.extent = Vk::toVkExtent2D(g_ctx.vk.swapChainImages[0]->extent);          \
        VkPipelineViewportStateCreateInfo viewportState {};                          \
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; \
        viewportState.viewportCount = 1;                                             \
        viewportState.pViewports = &viewport;                                        \
        viewportState.scissorCount = 1;                                              \
        viewportState.pScissors = &scissor;                                          \

template <typename T>
struct Pipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    T param;
    Vk::Buffer param_buf;

    void destroy()
    {
        if (layout != VK_NULL_HANDLE) {
            assert(pipeline != VK_NULL_HANDLE);
            vkDestroyPipelineLayout(g_ctx.vk.device, layout, nullptr);
            layout = VK_NULL_HANDLE;
            vkDestroyPipeline(g_ctx.vk.device, pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        } else {
            assert(pipeline == VK_NULL_HANDLE);
        }
        if (param_buf.id != uuid::nil_uuid()) {
            Vk::Buffer::Delete(g_ctx.vk, param_buf);
        }
    }

    static VkPipelineInputAssemblyStateCreateInfo inputAssemblyDefault()
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }
    static VkPipelineRasterizationStateCreateInfo rasterizationDefault()
    {
        VkPipelineRasterizationStateCreateInfo rasterizer {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f; // if it's not fill mode
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
        return rasterizer;
    }
    static VkPipelineMultisampleStateCreateInfo multisampleDefault()
    {
        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional
        return multisampling;
    }
    static VkPipelineShaderStageCreateInfo shaderStageDefault(VkShaderModule shaderModule, VkShaderStageFlagBits stage)
    {
        VkPipelineShaderStageCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = stage;
        info.module = shaderModule;
        info.pName = "main";
        return info;
    }

    void initLayout(const std::vector<VkDescriptorSetLayout>& layouts)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        if (vkCreatePipelineLayout(g_ctx.vk.device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
};
