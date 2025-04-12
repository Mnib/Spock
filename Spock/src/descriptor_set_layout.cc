#include <vulkan/vulkan_core.h>

#include "spock/descriptor_set_layout.hxx"
#include "spock/vulkan.hh"

namespace spock
{
    DescriptorSetLayout::DescriptorSetLayout(VkDescriptorSetLayout descriptor_set_layout)
        : m_DescriptorSetLayout(descriptor_set_layout) {
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(s_VulkanContext.Device, m_DescriptorSetLayout, nullptr);
    }
} // namespace spock
