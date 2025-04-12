#include <vulkan/vulkan_core.h>

#include "spock/buffers.hxx"
#include "spock/vulkan.hh"

namespace spock
{
    Buffer::Buffer(VkBuffer buffer, VkDeviceMemory buffer_memory, VkDeviceSize size)
        : m_Buffer(buffer)
        , m_BufferMemory(buffer_memory)
        , m_BufferSize(size) {
    }

    Buffer::~Buffer() {
        vkDestroyBuffer(s_VulkanContext.Device, m_Buffer, nullptr);
        vkFreeMemory(s_VulkanContext.Device, m_BufferMemory, nullptr);
    }
} // namespace spock
