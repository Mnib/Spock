#include <cstring>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "spock/spock.hh"
#include "spock/vulkan.hh"
#include "spock/window.hh"

namespace spock
{
#ifdef NDEBUG
    const bool s_EnableValidationLayers = false;
#else
    const bool s_EnableValidationLayers = true;
#endif

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator) {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void Spock::Initialize(const SpockSettings &settings) {
        s_VulkanContext.Win = std::make_unique<Window>(1920, 1080, "Test app", false);
        s_VulkanContext.Settings = settings;

        // Device creation
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();

        // Swapchain creation
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateColorResources();
        CreateDepthResources();
        CreateFramebuffers();
        CreateSyncObjects();

        // Command buffers and descriptor pool
        CreateCommandBuffers();
        CreateDescriptorPool();

        // UI
        InitImGUI();
    }

    void Spock::CreateDescriptorPool() {
        static constexpr const uint32_t MAX_COUNT = 1000 * MAX_FRAMES_IN_FLIGHT;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = MAX_COUNT;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = MAX_COUNT;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[2].descriptorCount = MAX_COUNT;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        pool_info.pPoolSizes = poolSizes.data();
        pool_info.maxSets = static_cast<uint32_t>(MAX_COUNT);

        if (vkCreateDescriptorPool(s_VulkanContext.Device, &pool_info, nullptr, &s_VulkanContext.DescriptorPool)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void Spock::CreateCommandBuffers() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = s_VulkanContext.CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = s_VulkanContext.CommandBuffers.size();

        if (vkAllocateCommandBuffers(s_VulkanContext.Device, &allocInfo, s_VulkanContext.CommandBuffers.data())
            != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkCommandBuffer Spock::BeginFrame() {
        auto result = AcquireNextImage(s_VulkanContext.CurrentImageIndex);
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        auto command_buffer = s_VulkanContext.CommandBuffers[s_VulkanContext.CurrentFrame];

        vkResetCommandBuffer(command_buffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = s_VulkanContext.RenderPass;
        renderPassInfo.framebuffer = s_VulkanContext.SwapChainFramebuffers[s_VulkanContext.CurrentImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = s_VulkanContext.SwapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0, 0, 0, 1.f}};
        clearValues[1].depthStencil = {1.f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Begin
        vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(s_VulkanContext.SwapChainExtent.width);
        viewport.height = static_cast<float>(s_VulkanContext.SwapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = s_VulkanContext.SwapChainExtent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        return command_buffer;
    }

    void Spock::EndFrame(VkCommandBuffer command_buffer) {
        vkCmdEndRenderPass(command_buffer);

        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        SubmitCommandBuffer(command_buffer, s_VulkanContext.CurrentImageIndex);
    }

    void Spock::WaitForIdle() {
        vkDeviceWaitIdle(s_VulkanContext.Device);
    }

    void Spock::Cleanup() {
        CleanupImGUI();

        vkFreeCommandBuffers(s_VulkanContext.Device, s_VulkanContext.CommandPool, s_VulkanContext.CommandBuffers.size(),
                             s_VulkanContext.CommandBuffers.data());
        vkDestroyDescriptorPool(s_VulkanContext.Device, s_VulkanContext.DescriptorPool, nullptr);

        CleanupSwapchain();

        vkDestroyRenderPass(s_VulkanContext.Device, s_VulkanContext.RenderPass, nullptr);

        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(s_VulkanContext.Device, s_VulkanContext.RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(s_VulkanContext.Device, s_VulkanContext.ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(s_VulkanContext.Device, s_VulkanContext.InFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(s_VulkanContext.Device, s_VulkanContext.CommandPool, nullptr);

        vkDestroyDevice(s_VulkanContext.Device, nullptr);

        if (s_EnableValidationLayers)
            DestroyDebugUtilsMessengerEXT(s_VulkanContext.Instance, s_VulkanContext.DebugMessenger, nullptr);

        vkDestroySurfaceKHR(s_VulkanContext.Instance, s_VulkanContext.Surface, nullptr);
        vkDestroyInstance(s_VulkanContext.Instance, nullptr);
    }

    std::unique_ptr<Window> &Spock::GetWindow() {
        return s_VulkanContext.Win;
    }

    uint32_t Spock::GetCurrentFrame() {
        return s_VulkanContext.CurrentFrame;
    }
} // namespace spock
