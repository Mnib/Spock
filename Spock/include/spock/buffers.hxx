#pragma once

#include <cstring>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spock/spock.hh"
#include "spock/vulkan.hh"

namespace spock
{
    class Buffer {
      public:
        Buffer(VkBuffer buffer, VkDeviceMemory buffer_memory, VkDeviceSize size);
        Buffer(const Buffer &) = delete;
        Buffer operator=(const Buffer &) = delete;
        ~Buffer();

        VkBuffer GetBuffer() const {
            return m_Buffer;
        }

      public:
        template <typename T>
        static std::unique_ptr<Buffer> CreateVertexBuffer(const std::vector<T> &vertices);

      private:
        VkBuffer m_Buffer;
        VkDeviceMemory m_BufferMemory;
        VkDeviceSize m_BufferSize;
    };

    template <typename T>
    std::unique_ptr<Buffer> Buffer::CreateVertexBuffer(const std::vector<T> &vertices) {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        Spock::CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                            stagingBufferMemory);

        void *data;
        vkMapMemory(s_VulkanContext.Device, stagingBufferMemory, 0, buffer_size, 0, &data);
        memcpy(data, vertices.data(), buffer_size);
        vkUnmapMemory(s_VulkanContext.Device, stagingBufferMemory);

        VkBuffer buffer;
        VkDeviceMemory buffer_memory;
        Spock::CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, buffer_memory);

        Spock::CopyBuffer(stagingBuffer, buffer, buffer_size);

        vkDestroyBuffer(s_VulkanContext.Device, stagingBuffer, nullptr);
        vkFreeMemory(s_VulkanContext.Device, stagingBufferMemory, nullptr);

        return std::make_unique<Buffer>(buffer, buffer_memory, buffer_size);
    }
} // namespace spock
