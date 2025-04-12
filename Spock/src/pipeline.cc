#include <cassert>
#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spock/pipeline.hh"
#include "spock/vulkan.hh"

namespace spock
{
    PipelineStage::PipelineStage(VkShaderModule shader_module, VkPipelineShaderStageCreateInfo shader_stage_create_info)
        : m_ShaderModule(shader_module)
        , m_ShaderStage(shader_stage_create_info) {
    }

    PipelineStage::~PipelineStage() {
        if (m_ShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(s_VulkanContext.Device, m_ShaderModule, nullptr);
    }

    PipelineStage::PipelineStage(PipelineStage &&other) noexcept
        : m_ShaderModule(other.m_ShaderModule)
        , m_ShaderStage(other.m_ShaderStage) {
        // Set the shader module to null to avoid freeing it in the destructor
        other.m_ShaderModule = VK_NULL_HANDLE;
    }

    PipelineStage PipelineStage::PipelineStageFromData(const uint32_t *code, size_t size, VkShaderStageFlagBits stage) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = size;
        createInfo.pCode = code;

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(s_VulkanContext.Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = stage;
        vertShaderStageInfo.module = shaderModule;
        vertShaderStageInfo.pName = "main";

        return PipelineStage{shaderModule, vertShaderStageInfo};
    }

    PipelineStage PipelineStage::PipelineStageFromFile(const std::string &path, VkShaderStageFlagBits stage) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t file_size = (size_t)file.tellg();
        std::vector<char> shader_code(file_size);

        file.seekg(0);
        file.read(shader_code.data(), file_size);

        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shader_code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(s_VulkanContext.Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = stage;
        vertShaderStageInfo.module = shaderModule;
        vertShaderStageInfo.pName = "main";

        return PipelineStage{shaderModule, vertShaderStageInfo};
    }

    std::unique_ptr<Pipeline> Pipeline::CreatePipeline(PipelineConfig &&pipeline_config) {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = s_VulkanContext.SwapChainExtent.width;
        viewport.height = s_VulkanContext.SwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = s_VulkanContext.SwapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Pipeline stages
        std::vector<VkPipelineShaderStageCreateInfo> pipelineStages{};
        pipelineStages.reserve(pipeline_config.Stages.size());
        for (const auto &s : pipeline_config.Stages) {
            pipelineStages.emplace_back(s.GetShaderStage());
        }

        // Binding descriptions
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &pipeline_config.BindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(pipeline_config.AttributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = pipeline_config.AttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = pipeline_config.Topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = s_VulkanContext.MaxUsableSamples;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = pipeline_config.DescriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = pipeline_config.DescriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = pipeline_config.PushConstants.size();
        pipelineLayoutInfo.pPushConstantRanges = pipeline_config.PushConstants.data();

        VkPipelineLayout pipeline_layout;
        if (vkCreatePipelineLayout(s_VulkanContext.Device, &pipelineLayoutInfo, nullptr, &pipeline_layout)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = pipelineStages.size();
        pipelineInfo.pStages = pipelineStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipeline_layout;
        pipelineInfo.renderPass = s_VulkanContext.RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(
                s_VulkanContext.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        return std::make_unique<Pipeline>(graphics_pipeline, pipeline_layout);
    }

    void Pipeline::Bind(VkCommandBuffer command_buffer) const {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }

    Pipeline::Pipeline(VkPipeline pipeline, VkPipelineLayout pipeline_layout)
        : m_Pipeline(pipeline)
        , m_PipelineLayout(pipeline_layout) {
    }

    Pipeline::~Pipeline() {
        vkDestroyPipeline(s_VulkanContext.Device, m_Pipeline, nullptr);
        vkDestroyPipelineLayout(s_VulkanContext.Device, m_PipelineLayout, nullptr);
    }
} // namespace spock
