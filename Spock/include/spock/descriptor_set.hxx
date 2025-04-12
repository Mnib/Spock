#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "spock/descriptor.hxx"
#include "spock/descriptor_set_layout.hxx"
#include "spock/vulkan.hh"

namespace spock
{
    using DescriptorSet = VkDescriptorSet;

    template <std::size_t Nm>
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>
    CreateDescriptorSets(const std::unique_ptr<DescriptorSetLayout> &descriptor_set_layout,
                         std::array<Descriptor *, Nm> descriptors) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                   descriptor_set_layout->GetDescriptorSetLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = s_VulkanContext.DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets{};
        if (vkAllocateDescriptorSets(s_VulkanContext.Device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, Nm> descriptor_writes{};
            std::array<VkDescriptorBufferInfo, Nm> buffer_infos{};
            std::array<VkDescriptorImageInfo, Nm> image_infos{};

            for (size_t j = 0; j < Nm; j++) {
                descriptor_writes[j] =
                    descriptors[j]->GetWriteDescriptorSet(i, descriptor_sets[i], buffer_infos[j], image_infos[j]);
            }

            vkUpdateDescriptorSets(s_VulkanContext.Device, static_cast<uint32_t>(descriptor_writes.size()),
                                   descriptor_writes.data(), 0, nullptr);
        }

        return descriptor_sets;
    }
} // namespace spock
