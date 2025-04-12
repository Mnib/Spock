#pragma once

#include "spock/vulkan.hh"
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace spock
{
    class DescriptorSetLayout {
      public:
        DescriptorSetLayout(VkDescriptorSetLayout descriptor_set_layout);
        DescriptorSetLayout(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout operator=(const DescriptorSetLayout &) = delete;
        ~DescriptorSetLayout();
        void Cleanup();

        VkDescriptorSetLayout GetDescriptorSetLayout() const {
            return m_DescriptorSetLayout;
        }

      public:
        template <std::size_t Nm>
        static std::unique_ptr<DescriptorSetLayout>
        CreateDescriptorSetLayout(const std::array<VkDescriptorSetLayoutBinding, Nm> &bindings);

      private:
        VkDescriptorSetLayout m_DescriptorSetLayout;
    };

    template <std::size_t Nm>
    std::unique_ptr<DescriptorSetLayout>
    DescriptorSetLayout::CreateDescriptorSetLayout(const std::array<VkDescriptorSetLayoutBinding, Nm> &bindings) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout descriptor_set_layout;
        if (vkCreateDescriptorSetLayout(s_VulkanContext.Device, &layoutInfo, nullptr, &descriptor_set_layout)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        return std::make_unique<DescriptorSetLayout>(descriptor_set_layout);
    }
} // namespace spock
