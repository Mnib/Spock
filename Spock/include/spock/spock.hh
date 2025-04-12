#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace spock
{
    struct SpockSettings
    {
        SpockSettings() = default;

        // Your vulkan application name
        const char *ApplicationName = "Spock Application";

        // Engine name
        const char *EngineName = "No engine";

        uint32_t ApplicationVersion = VK_MAKE_VERSION(1, 0, 0);
        uint32_t EngineVersion = VK_MAKE_VERSION(1, 0, 0);

        // Defaults to Vulkan 1.3
        uint32_t ApiVersion = VK_API_VERSION_1_3;

        // Choose `VK_PRESENT_MODE_FIFO_KHR` for V-Sync
        std::vector<VkPresentModeKHR> PresentModes = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR,
                                                      VK_PRESENT_MODE_FIFO_KHR};
    };

    class Window;

    class Spock {
      public:
        // Lifecycle
        static void Initialize(const SpockSettings &settings);
        static void Cleanup();
        static void WaitForIdle();

        // Rendering
        static VkCommandBuffer BeginFrame();
        static void EndFrame(VkCommandBuffer command_buffer);
        static uint32_t GetCurrentFrame();

        // Utils
        static std::unique_ptr<Window> &GetWindow();

      private:
        static void CreateInstance();
        static void SetupDebugMessenger();
        static void CreateSurface();
        static void PickPhysicalDevice();
        static void CreateLogicalDevice();
        static void CreateCommandPool();

        static void CreateSwapchain();
        static void CreateImageViews();
        static void CreateRenderPass();
        static void CreateColorResources();
        static void CreateDepthResources();
        static void CreateFramebuffers();
        static void CreateSyncObjects();

        static void CreateCommandBuffers();
        static void CreateDescriptorPool();

        // Cleanup functions
        static void CleanupSwapchain();
        static void RecreateSwapchain();

        // UI
        static void InitImGUI();
        static void CleanupImGUI();

      public:
        static VkResult AcquireNextImage(uint32_t &image_index);
        static VkResult SubmitCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
        static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
                                           uint32_t mip_levels);
        static void CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format,
                                VkSampleCountFlagBits num_samples, VkImageTiling tiling, VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);
        static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                 VkBuffer &buffer, VkDeviceMemory &buffer_memory);
        static void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout,
                                          VkImageLayout new_layout, uint32_t mip_levels);

        static VkCommandBuffer BeginSingleTimeCommands();
        static void EndSingleTimeCommands(VkCommandBuffer command_buffer);
    };
} // namespace spock
