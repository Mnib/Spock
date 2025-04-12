#pragma once

#include <memory>
#include <vulkan/vulkan_core.h>

#include "spock/texture.hh"
#include "spock/uniform_buffer.hxx"

namespace spock
{
    class Descriptor {
      public:
        Descriptor(int binding)
            : m_Binding(binding) {};
        virtual VkWriteDescriptorSet GetWriteDescriptorSet(int frame_index, VkDescriptorSet dstSet,
                                                           VkDescriptorBufferInfo &buffer_info,
                                                           VkDescriptorImageInfo &image_info) = 0;

      protected:
        int m_Binding;
    };

    template <typename T>
    class UniformBufferDescriptor : public Descriptor {
      public:
        UniformBufferDescriptor(int binding, const std::unique_ptr<UniformBuffer<T>> &uniform_buffer)
            : Descriptor(binding)
            , m_UniformBuffer(uniform_buffer) {
        }

        virtual VkWriteDescriptorSet GetWriteDescriptorSet(int frame_index, VkDescriptorSet dstSet,
                                                           VkDescriptorBufferInfo &buffer_info,
                                                           VkDescriptorImageInfo &) override {
            buffer_info.buffer = m_UniformBuffer->GetBuffer(frame_index);
            buffer_info.offset = 0;
            buffer_info.range = sizeof(T);

            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = dstSet;
            descriptor_write.dstBinding = m_Binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;

            return descriptor_write;
        }

      private:
        const std::unique_ptr<UniformBuffer<T>> &m_UniformBuffer;
    };

    class ImageSamplerDescriptor : public Descriptor {
      public:
        ImageSamplerDescriptor(int binding, const std::shared_ptr<Texture2D> &texture)
            : Descriptor(binding)
            , m_Texture(texture) {
        }

        virtual VkWriteDescriptorSet GetWriteDescriptorSet(int, VkDescriptorSet dstSet, VkDescriptorBufferInfo &,
                                                           VkDescriptorImageInfo &image_info) override {
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = m_Texture->GetImageView();
            image_info.sampler = m_Texture->GetSampler();

            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = dstSet;
            descriptor_write.dstBinding = m_Binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pImageInfo = &image_info;

            return descriptor_write;
        }

      private:
        const std::shared_ptr<Texture2D> &m_Texture;
    };
} // namespace spock
