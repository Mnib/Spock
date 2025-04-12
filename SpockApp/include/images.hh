#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "spock/buffers.hxx"
#include "spock/descriptor_set.hxx"
#include "spock/descriptor_set_layout.hxx"
#include "spock/pipeline.hh"
#include "spock/texture.hh"
#include "spock/uniform_buffer.hxx"

class ExampleImage {
  private:
    struct UniformBufferObject
    {
        glm::mat4 Model;
        glm::mat4 View;
        glm::mat4 Projection;
    };

  public:
    ExampleImage();
    ExampleImage(const ExampleImage &) = delete;
    ExampleImage operator=(const ExampleImage &) = delete;

    void Update(float rotation);
    void Render(VkCommandBuffer command_buffer) const;

  private:
    std::unique_ptr<spock::Pipeline> m_Pipeline;
    std::unique_ptr<spock::DescriptorSetLayout> m_DescriptorSetLayout;
    std::array<spock::DescriptorSet, spock::MAX_FRAMES_IN_FLIGHT> m_DescriptorSets;
    std::unique_ptr<spock::UniformBuffer<UniformBufferObject>> m_UniformBuffer;
    std::shared_ptr<spock::Texture2D> m_Texture;
    std::unique_ptr<spock::Buffer> m_VertexBuffer;
};
