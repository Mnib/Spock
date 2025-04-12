#pragma once

#include <array>
#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>

#include "spock/vulkan.hh"

namespace spock
{
    template <typename T>
    class UniformBuffer {
      public:
        UniformBuffer(std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> &&uniform_buffers,
                      std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> &&uniform_buffers_memory,
                      std::array<void *, MAX_FRAMES_IN_FLIGHT> &&uniform_buffers_mapped);
        ~UniformBuffer();

        UniformBuffer(const UniformBuffer &) = delete;
        UniformBuffer operator=(const UniformBuffer &) = delete;
        UniformBuffer &operator=(UniformBuffer &&) noexcept;

        void SetData(const T &data);

        VkBuffer GetBuffer(int id) const {
            return m_UniformBuffers[id];
        }

      public:
        static std::unique_ptr<UniformBuffer<T>> CreateUniformBuffer();

      private:
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_UniformBuffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_UniformBuffersMemory;
        std::array<void *, MAX_FRAMES_IN_FLIGHT> m_UniformBuffersMapped;
    };

    template <typename T>
    std::unique_ptr<UniformBuffer<T>> UniformBuffer<T>::CreateUniformBuffer() {
        VkDeviceSize buffer_size = sizeof(T);

        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uniform_buffers_memory;
        std::array<void *, MAX_FRAMES_IN_FLIGHT> uniform_buffers_mapped;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            Spock::CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                uniform_buffers[i], uniform_buffers_memory[i]);

            vkMapMemory(s_VulkanContext.Device, uniform_buffers_memory[i], 0, buffer_size, 0,
                        &uniform_buffers_mapped[i]);
        }

        return std::make_unique<UniformBuffer<T>>(std::move(uniform_buffers), std::move(uniform_buffers_memory),
                                                  std::move(uniform_buffers_mapped));
    }

    template <typename T>
    void UniformBuffer<T>::SetData(const T &data) {
        memcpy(m_UniformBuffersMapped[s_VulkanContext.CurrentFrame], &data, sizeof(T));
    }

    template <typename T>
    UniformBuffer<T>::UniformBuffer(std::array<VkBuffer, 2> &&uniform_buffers,
                                    std::array<VkDeviceMemory, 2> &&uniform_buffers_memory,
                                    std::array<void *, 2> &&uniform_buffers_mapped)
        : m_UniformBuffers(std::move(uniform_buffers))
        , m_UniformBuffersMemory(std::move(uniform_buffers_memory))
        , m_UniformBuffersMapped(std::move(uniform_buffers_mapped)) {
    }

    template <typename T>
    UniformBuffer<T>::~UniformBuffer() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(s_VulkanContext.Device, m_UniformBuffers[i], nullptr);
            vkFreeMemory(s_VulkanContext.Device, m_UniformBuffersMemory[i], nullptr);
        }
    }
} // namespace spock
