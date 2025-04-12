#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include "images.hh"
#include "spock/descriptor.hxx"
#include "spock/texture.hh"

struct ImageVertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(ImageVertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(2, VkVertexInputAttributeDescription{});

        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(ImageVertex, Position);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(ImageVertex, TexCoord);

        return attribute_descriptions;
    }

    bool operator==(const ImageVertex &other) const {
        return Position == other.Position && TexCoord == other.TexCoord;
    }
};

ExampleImage::ExampleImage() {
    // Shader stages
    std::vector<spock::PipelineStage> stages;
    stages.emplace_back(spock::PipelineStage::PipelineStageFromFile("SpockApp/resources/shaders/textures.vert.spv",
                                                                    VK_SHADER_STAGE_VERTEX_BIT));
    stages.emplace_back(spock::PipelineStage::PipelineStageFromFile("SpockApp/resources/shaders/textures.frag.spv",
                                                                    VK_SHADER_STAGE_FRAGMENT_BIT));

    // Setup the uniform buffers
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    m_DescriptorSetLayout = spock::DescriptorSetLayout::CreateDescriptorSetLayout(bindings);

    m_UniformBuffer = spock::UniformBuffer<UniformBufferObject>::CreateUniformBuffer();
    m_Texture = spock::Texture2D::FromFile("SpockApp/resources/images/texture.jpg");

    // Add our descriptors to the set
    auto uniform_buffer_descriptor = spock::UniformBufferDescriptor(0, m_UniformBuffer);
    auto texture_descriptor = spock::ImageSamplerDescriptor(1, m_Texture);
    std::array<spock::Descriptor *, 2> descriptors{&uniform_buffer_descriptor, &texture_descriptor};
    m_DescriptorSets = spock::CreateDescriptorSets(m_DescriptorSetLayout, std::move(descriptors));

    // Generate the pipeline config
    spock::PipelineConfig pipeline_config{};
    pipeline_config.Stages = std::move(stages);
    pipeline_config.BindingDescription = ImageVertex::GetBindingDescription();
    pipeline_config.AttributeDescriptions = ImageVertex::GetAttributeDescriptions();
    pipeline_config.DescriptorSetLayouts = {m_DescriptorSetLayout->GetDescriptorSetLayout()};

    m_Pipeline = spock::Pipeline::CreatePipeline(std::move(pipeline_config));

    // Load the vertices
    auto vertices = std::vector<ImageVertex>();
    // clang-format off
    vertices.emplace_back(ImageVertex{glm::vec3{-0.8, -0.8, 0.4f}, glm::vec2{0, 0}});
    vertices.emplace_back(ImageVertex{glm::vec3{ 0.8, -0.8, 0.4f}, glm::vec2{0, 1}});
    vertices.emplace_back(ImageVertex{glm::vec3{ 0.8,  0.8, 0.4f}, glm::vec2{1, 1}});

    vertices.emplace_back(ImageVertex{glm::vec3{ 0.8,  0.8, 0.4f}, glm::vec2{1, 1}});
    vertices.emplace_back(ImageVertex{glm::vec3{-0.8,  0.8, 0.4f}, glm::vec2{1, 0}});
    vertices.emplace_back(ImageVertex{glm::vec3{-0.8, -0.8, 0.4f}, glm::vec2{0, 0}});
    // clang-format on

    m_VertexBuffer = spock::Buffer::CreateVertexBuffer<ImageVertex>(vertices);
}

void ExampleImage::Update(float rotation) {
    UniformBufferObject ubo{};
    ubo.Model =
        glm::rotate(glm::mat4(1.0f), (6.f / 60000) * rotation * glm::radians(360.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0, 0, 0), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Projection = glm::perspective(glm::radians(45.0f), 16 / (float)9, 0.1f, 1000.0f); // 45deg fov, 16:9 ratio
    ubo.Projection[1][1] *= -1;

    m_UniformBuffer->SetData(ubo);
}

void ExampleImage::Render(VkCommandBuffer command_buffer) const {
    m_Pipeline->Bind(command_buffer);

    // Bind the vertex buffer
    VkDeviceSize offset[] = {0};
    VkBuffer vertex_buffers[] = {m_VertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offset);

    // Bind the uniform buffer
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetLayout(), 0, 1,
                            &m_DescriptorSets[spock::Spock::GetCurrentFrame()], 0, nullptr);

    vkCmdDraw(command_buffer, 6, 1, 0, 0);
}
