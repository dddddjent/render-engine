#include "./node.h"
#include "core/filesystem/file.h"
#include "core/vulkan/vulkan_util.h"
#include "function/global_context.h"
#include "function/render/render_graph/pipeline.hpp"

using namespace Vk;

CalculateLuminance::CalculateLuminance(
    const std::string& name,
    const std::string& sdr_buf,
    const std::string& sdr_buf_alpha_illuminance)
    : RenderGraphNode(name)
{
    bool is_swapchain = sdr_buf_alpha_illuminance == RenderAttachmentDescription::SWAPCHAIN_IMAGE_NAME();

    attachment_descriptions = {
        {
            "sdr",
            {
                sdr_buf,
                RenderAttachmentType::Color | RenderAttachmentType::Sampler,
                RenderAttachmentRW::Read,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_FORMAT_B8G8R8A8_UNORM,
            },
        },
        {
            "sdr_alpha_illuminance",
            RenderAttachmentDescription {
                sdr_buf_alpha_illuminance,
                RenderAttachmentType::Color,
                RenderAttachmentRW::Write,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                is_swapchain ? g_ctx.vk.swapChainImages[0]->format : VK_FORMAT_B8G8R8A8_UNORM,
            },
        }
    };
}

void CalculateLuminance::init(Configuration& cfg, RenderAttachments& attachments)
{
    this->attachments = &attachments;
    createRenderPass();
    createFramebuffer();
    createPipeline(cfg);
}

void CalculateLuminance::createFramebuffer()
{
    framebuffers.resize(g_ctx.vk.swapChainImages.size());
    for (int i = 0; i < g_ctx.vk.swapChainImages.size(); i++) {
        std::array<VkImageView, 1> views = {
            getAttachmentByName(attachment_descriptions["sdr_alpha_illuminance"].name, attachments, i)->view
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
        framebufferInfo.pAttachments = views.data();
        framebufferInfo.width = g_ctx.vk.swapChainImages[i]->extent.width;
        framebufferInfo.height = g_ctx.vk.swapChainImages[i]->extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(g_ctx.vk.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void CalculateLuminance::createRenderPass()
{
    std::vector<AttachmentDescriptionHelper> helpers = {
        { "sdr_alpha_illuminance", VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE },
    };

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass = DefaultRenderPass(attachment_descriptions, helpers, dependency);
}

void CalculateLuminance::createPipeline(Configuration& cfg)
{
    {
        std::vector<VkDescriptorSetLayout> descLayouts = {
            g_ctx.dm.BINDLESS_LAYOUT(),
            g_ctx.dm.PARAMETER_LAYOUT(),
        };
        pipeline.initLayout(descLayouts);
    }

    {
        VertexInputDefault(false);
        DynamicStateDefault();
        ViewportStateDefault();
        auto inputAssembly = Pipeline<Param>::inputAssemblyDefault();
        auto rasterization = Pipeline<Param>::rasterizationDefault();
        auto multisample = Pipeline<Param>::multisampleDefault();

        auto vertShaderCode = readFile(cfg.at("shader_directory").get<std::string>() + "/calculate_luminance/node.vert.spv");
        auto fragShaderCode = readFile(cfg.at("shader_directory").get<std::string>() + "/calculate_luminance/node.frag.spv");
        auto vertShaderModule = createShaderModule(g_ctx.vk, vertShaderCode);
        auto fragShaderModule = createShaderModule(g_ctx.vk, fragShaderCode);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
            Pipeline<Param>::shaderStageDefault(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT),
            Pipeline<Param>::shaderStageDefault(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT),
        };
        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        VkPipelineColorBlendStateCreateInfo colorBlending {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        VkPipelineDepthStencilStateCreateInfo depthStencil {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterization;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipeline.layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional
        if (vkCreateGraphicsPipelines(g_ctx.vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        vkDestroyShaderModule(g_ctx.vk.device, vertShaderModule, nullptr);
        vkDestroyShaderModule(g_ctx.vk.device, fragShaderModule, nullptr);
    }

    {
        pipeline.param.sdr_img = g_ctx.dm.getResourceHandle(
            attachments->getAttachment(attachment_descriptions["sdr"].name).id);
        pipeline.param_buf = Buffer::New(
            g_ctx.vk,
            sizeof(Param),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true);
        pipeline.param_buf.Update(g_ctx.vk, &pipeline.param, sizeof(Param));
        g_ctx.dm.registerParameter(pipeline.param_buf);
    }
}

void CalculateLuminance::record(uint32_t swapchain_index)
{
    setDefaultViewportAndScissor();

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = framebuffers[swapchain_index];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = toVkExtent2D(g_ctx.vk.swapChainImages[swapchain_index]->extent);
    renderPassInfo.clearValueCount = 0;
    renderPassInfo.pClearValues = nullptr;
    vkCmdBeginRenderPass(g_ctx.vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(g_ctx.vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    bindDescriptorSet(0, pipeline.layout, g_ctx.dm.BINDLESS_SET());
    bindDescriptorSet(1, pipeline.layout, g_ctx.dm.getParameterSet(pipeline.param_buf.id));

    vkCmdDraw(g_ctx.vk.commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(g_ctx.vk.commandBuffer);
}

void CalculateLuminance::onResize()
{
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_ctx.vk.device, framebuffer, nullptr);
    }
    createFramebuffer();
}

void CalculateLuminance::destroy()
{
    pipeline.destroy();
    vkDestroyRenderPass(g_ctx.vk.device, render_pass, nullptr);
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_ctx.vk.device, framebuffer, nullptr);
    }
}
