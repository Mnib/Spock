#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace spock
{
    class PipelineStage {
      public:
        PipelineStage(VkShaderModule shader_module, VkPipelineShaderStageCreateInfo shader_stage_create_info);
        PipelineStage(const PipelineStage &) = delete;
        PipelineStage operator=(const PipelineStage &) = delete;
        PipelineStage(PipelineStage &&other) noexcept;
        ~PipelineStage();

        VkPipelineShaderStageCreateInfo GetShaderStage() const {
            return m_ShaderStage;
        }

      public:
        static PipelineStage PipelineStageFromFile(const std::string &path, VkShaderStageFlagBits stage);
        static PipelineStage PipelineStageFromData(const uint32_t *code, size_t size, VkShaderStageFlagBits stage);

      private:
        VkShaderModule m_ShaderModule;
        VkPipelineShaderStageCreateInfo m_ShaderStage;
    };

    struct PipelineConfig
    {
        PipelineConfig() = default;
        PipelineConfig(const PipelineConfig &) = delete;
        PipelineConfig operator=(const PipelineConfig &) = delete;
        PipelineConfig(PipelineConfig &&) = default;

        std::vector<PipelineStage> Stages;
        std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
        VkVertexInputBindingDescription BindingDescription;
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
        std::vector<VkPushConstantRange> PushConstants;
        VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    class Pipeline {
      public:
        Pipeline(VkPipeline pipeline, VkPipelineLayout pipeline_layout);
        Pipeline(const Pipeline &) = delete;
        Pipeline operator=(const Pipeline &) = delete;
        ~Pipeline();

        static std::unique_ptr<Pipeline> CreatePipeline(PipelineConfig &&pipeline_config);

        void Bind(VkCommandBuffer command_buffer) const;
        VkPipelineLayout GetLayout() const {
            return m_PipelineLayout;
        }

      private:
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
    };
} // namespace spock
