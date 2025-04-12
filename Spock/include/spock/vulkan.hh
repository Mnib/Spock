#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spock/spock.hh"
#include "spock/window.hh"

namespace spock
{
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;

        bool IsComplete() {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    struct VulkanContext
    {
        std::unique_ptr<Window> Win;
        SpockSettings Settings;

        // Vulkan device stuff
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface;
        VkPhysicalDevice PhysicalDevice;
        VkPhysicalDeviceProperties PhysicalDeviceProperties;
        VkSampleCountFlagBits MaxUsableSamples;
        VkDevice Device;
        VkQueue GraphicsQueue;
        VkQueue PresentQueue;
        VkCommandPool CommandPool;

        // Swapchain stuff
        VkSwapchainKHR SwapChain;
        VkExtent2D SwapChainExtent;
        VkFormat SwapChainImageFormat;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;
        VkRenderPass RenderPass;
        VkImage DepthImage;
        VkDeviceMemory DepthImageMemory;
        VkImageView DepthImageView;
        VkImage ColorImage;
        VkDeviceMemory ColorImageMemory;
        VkImageView ColorImageView;
        std::vector<VkFramebuffer> SwapChainFramebuffers;
        std::vector<VkSemaphore> ImageAvailableSemaphores;
        std::vector<VkSemaphore> RenderFinishedSemaphores;
        std::vector<VkFence> InFlightFences;
        uint32_t CurrentFrame = 0;
        uint32_t CurrentImageIndex = 0;

        // Rendering stuff
        VkDescriptorPool DescriptorPool;
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> CommandBuffers;
    };

    inline VulkanContext s_VulkanContext{};
} // namespace spock
