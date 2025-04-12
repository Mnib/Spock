#include <GLFW/glfw3.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spock/spock.hh"
#include "spock/vulkan.hh"
#include "spock/window.hh"

namespace spock
{
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
                && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes,
                                                  const std::vector<VkPresentModeKHR> &wanted_present_modes) {
        for (const auto &wanted_present_mode : wanted_present_modes) {
            for (const auto &availablePresentMode : availablePresentModes) {
                if (availablePresentMode == wanted_present_mode) {
                    return availablePresentMode;
                }
            }
        }

        // V-Sync as fallback
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D ChooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    static VkFormat FindSupportedFormat(VkPhysicalDevice physical_device, const std::vector<VkFormat> &candidates,
                                        VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    void Spock::CreateSwapchain() {
        SwapChainSupportDetails swapChainSupport =
            QuerySwapChainSupport(s_VulkanContext.PhysicalDevice, s_VulkanContext.Surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
        VkPresentModeKHR presentMode =
            ChooseSwapPresentMode(swapChainSupport.PresentModes, s_VulkanContext.Settings.PresentModes);
        VkExtent2D extent = ChooseSwapExtent(s_VulkanContext.Win->GetWindow(), swapChainSupport.Capabilities);

        uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
        if (swapChainSupport.Capabilities.maxImageCount > 0
            && imageCount > swapChainSupport.Capabilities.maxImageCount) {
            imageCount = swapChainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = s_VulkanContext.Surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = FindQueueFamilies(s_VulkanContext.PhysicalDevice, s_VulkanContext.Surface);
        uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

        if (indices.GraphicsFamily != indices.PresentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(s_VulkanContext.Device, &createInfo, nullptr, &s_VulkanContext.SwapChain)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(s_VulkanContext.Device, s_VulkanContext.SwapChain, &imageCount, nullptr);
        s_VulkanContext.SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(s_VulkanContext.Device, s_VulkanContext.SwapChain, &imageCount,
                                s_VulkanContext.SwapChainImages.data());

        s_VulkanContext.SwapChainImageFormat = surfaceFormat.format;
        s_VulkanContext.SwapChainExtent = extent;
    }

    void Spock::CreateImageViews() {
        s_VulkanContext.SwapChainImageViews.resize(s_VulkanContext.SwapChainImages.size());

        for (size_t i = 0; i < s_VulkanContext.SwapChainImages.size(); i++) {
            s_VulkanContext.SwapChainImageViews[i] = CreateImageView(
                s_VulkanContext.SwapChainImages[i], s_VulkanContext.SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void Spock::CreateRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = s_VulkanContext.SwapChainImageFormat;
        colorAttachment.samples = s_VulkanContext.MaxUsableSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format =
            FindSupportedFormat(s_VulkanContext.PhysicalDevice,
                                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depthAttachment.samples = s_VulkanContext.MaxUsableSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = s_VulkanContext.SwapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(s_VulkanContext.Device, &renderPassInfo, nullptr, &s_VulkanContext.RenderPass)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void Spock::CreateColorResources() {
        VkFormat colorFormat = s_VulkanContext.SwapChainImageFormat;

        CreateImage(s_VulkanContext.SwapChainExtent.width, s_VulkanContext.SwapChainExtent.height, 1,
                    s_VulkanContext.SwapChainImageFormat, s_VulkanContext.MaxUsableSamples, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, s_VulkanContext.ColorImage, s_VulkanContext.ColorImageMemory);
        s_VulkanContext.ColorImageView =
            CreateImageView(s_VulkanContext.ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void Spock::CreateDepthResources() {
        VkFormat depthFormat =
            FindSupportedFormat(s_VulkanContext.PhysicalDevice,
                                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        CreateImage(s_VulkanContext.SwapChainExtent.width, s_VulkanContext.SwapChainExtent.height, 1, depthFormat,
                    s_VulkanContext.MaxUsableSamples, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    s_VulkanContext.DepthImage, s_VulkanContext.DepthImageMemory);
        s_VulkanContext.DepthImageView =
            CreateImageView(s_VulkanContext.DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    void Spock::CreateFramebuffers() {
        s_VulkanContext.SwapChainFramebuffers.resize(s_VulkanContext.SwapChainImageViews.size());

        for (size_t i = 0; i < s_VulkanContext.SwapChainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {s_VulkanContext.ColorImageView, s_VulkanContext.DepthImageView,
                                                      s_VulkanContext.SwapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = s_VulkanContext.RenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = s_VulkanContext.SwapChainExtent.width;
            framebufferInfo.height = s_VulkanContext.SwapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(s_VulkanContext.Device, &framebufferInfo, nullptr,
                                    &s_VulkanContext.SwapChainFramebuffers[i])
                != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void Spock::CreateSyncObjects() {
        s_VulkanContext.ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_VulkanContext.RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        s_VulkanContext.InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(s_VulkanContext.Device, &semaphoreInfo, nullptr,
                                  &s_VulkanContext.ImageAvailableSemaphores[i])
                    != VK_SUCCESS
                || vkCreateSemaphore(s_VulkanContext.Device, &semaphoreInfo, nullptr,
                                     &s_VulkanContext.RenderFinishedSemaphores[i])
                       != VK_SUCCESS
                || vkCreateFence(s_VulkanContext.Device, &fenceInfo, nullptr, &s_VulkanContext.InFlightFences[i])
                       != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    void Spock::CleanupSwapchain() {
        vkDestroyImageView(s_VulkanContext.Device, s_VulkanContext.DepthImageView, nullptr);
        vkDestroyImage(s_VulkanContext.Device, s_VulkanContext.DepthImage, nullptr);
        vkFreeMemory(s_VulkanContext.Device, s_VulkanContext.DepthImageMemory, nullptr);

        vkDestroyImageView(s_VulkanContext.Device, s_VulkanContext.ColorImageView, nullptr);
        vkDestroyImage(s_VulkanContext.Device, s_VulkanContext.ColorImage, nullptr);
        vkFreeMemory(s_VulkanContext.Device, s_VulkanContext.ColorImageMemory, nullptr);

        for (auto framebuffer : s_VulkanContext.SwapChainFramebuffers) {
            vkDestroyFramebuffer(s_VulkanContext.Device, framebuffer, nullptr);
        }

        for (auto image_view : s_VulkanContext.SwapChainImageViews) {
            vkDestroyImageView(s_VulkanContext.Device, image_view, nullptr);
        }

        vkDestroySwapchainKHR(s_VulkanContext.Device, s_VulkanContext.SwapChain, nullptr);
    }

    void Spock::RecreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(s_VulkanContext.Win->GetWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(s_VulkanContext.Win->GetWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(s_VulkanContext.Device);

        CleanupSwapchain();

        CreateSwapchain();
        CreateImageViews();
        CreateColorResources();
        CreateDepthResources();
        CreateFramebuffers();
    }

    VkResult Spock::AcquireNextImage(uint32_t &image_index) {
        vkWaitForFences(s_VulkanContext.Device, 1, &s_VulkanContext.InFlightFences[s_VulkanContext.CurrentFrame],
                        VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(s_VulkanContext.Device, s_VulkanContext.SwapChain, UINT64_MAX,
                                                s_VulkanContext.ImageAvailableSemaphores[s_VulkanContext.CurrentFrame],
                                                VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return result;
        }

        vkResetFences(s_VulkanContext.Device, 1, &s_VulkanContext.InFlightFences[s_VulkanContext.CurrentFrame]);

        return result;
    }

    VkResult Spock::SubmitCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {s_VulkanContext.ImageAvailableSemaphores[s_VulkanContext.CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer;

        VkSemaphore signalSemaphores[] = {s_VulkanContext.RenderFinishedSemaphores[s_VulkanContext.CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(s_VulkanContext.GraphicsQueue, 1, &submitInfo,
                          s_VulkanContext.InFlightFences[s_VulkanContext.CurrentFrame])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {s_VulkanContext.SwapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &image_index;

        auto result = vkQueuePresentKHR(s_VulkanContext.PresentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR
            || s_VulkanContext.Win->WasFramebufferResized()) {
            s_VulkanContext.Win->SetFramebufferResized(false);
            RecreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        s_VulkanContext.CurrentFrame = (s_VulkanContext.CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }
} // namespace spock
