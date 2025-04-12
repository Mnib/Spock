#include <cstring>
#include <fmt/base.h>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
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

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDeviceProperties physical_device_properties) {
        VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts
                                    & physical_device_properties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT) {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT) {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT) {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT) {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT) {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    static bool CheckValidationLayerSupport() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char *layer_name : validationLayers) {
            bool layer_found = false;

            for (const auto &layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *) {
        fmt::println("validation layer: {}", pCallbackData->pMessage);

        return VK_FALSE;
    }

    static inline void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static std::vector<const char *> GetRequiredExtensions() {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (s_EnableValidationLayers) {
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    static bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices = FindQueueFamilies(device, surface);

        bool extensions_supported = CheckDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensions_supported) {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.IsComplete() && extensions_supported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    static uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter,
                                   VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Spock::CreateInstance() {
        if (s_EnableValidationLayers && !CheckValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = s_VulkanContext.Settings.ApplicationName;
        app_info.applicationVersion = s_VulkanContext.Settings.ApplicationVersion;
        app_info.pEngineName = s_VulkanContext.Settings.EngineName;
        app_info.engineVersion = s_VulkanContext.Settings.EngineVersion;
        app_info.apiVersion = s_VulkanContext.Settings.ApiVersion;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto extensions = GetRequiredExtensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        if (s_EnableValidationLayers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            create_info.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debug_create_info);
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;

            create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&create_info, nullptr, &s_VulkanContext.Instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void Spock::SetupDebugMessenger() {
        if (!s_EnableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        PopulateDebugMessengerCreateInfo(create_info);

        if (CreateDebugUtilsMessengerEXT(s_VulkanContext.Instance, &create_info, nullptr,
                                         &s_VulkanContext.DebugMessenger)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void Spock::CreateSurface() {
        if (glfwCreateWindowSurface(s_VulkanContext.Instance, s_VulkanContext.Win->GetWindow(), nullptr,
                                    &s_VulkanContext.Surface)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void Spock::PickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(s_VulkanContext.Instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(s_VulkanContext.Instance, &deviceCount, devices.data());

        for (const auto &device : devices) {
            if (IsDeviceSuitable(device, s_VulkanContext.Surface)) {
                s_VulkanContext.PhysicalDevice = device;
                vkGetPhysicalDeviceProperties(device, &s_VulkanContext.PhysicalDeviceProperties);
                s_VulkanContext.MaxUsableSamples = GetMaxUsableSampleCount(s_VulkanContext.PhysicalDeviceProperties);
                break;
            }
        }

        if (s_VulkanContext.PhysicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        fmt::println("Using {}", s_VulkanContext.PhysicalDeviceProperties.deviceName);
    }

    void Spock::CreateLogicalDevice() {
        QueueFamilyIndices indices = FindQueueFamilies(s_VulkanContext.PhysicalDevice, s_VulkanContext.Surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.emplace_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (s_EnableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(s_VulkanContext.PhysicalDevice, &createInfo, nullptr, &s_VulkanContext.Device)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(s_VulkanContext.Device, indices.GraphicsFamily.value(), 0, &s_VulkanContext.GraphicsQueue);
        vkGetDeviceQueue(s_VulkanContext.Device, indices.PresentFamily.value(), 0, &s_VulkanContext.PresentQueue);
    }

    void Spock::CreateCommandPool() {
        QueueFamilyIndices queueFamilyIndices =
            FindQueueFamilies(s_VulkanContext.PhysicalDevice, s_VulkanContext.Surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

        if (vkCreateCommandPool(s_VulkanContext.Device, &poolInfo, nullptr, &s_VulkanContext.CommandPool)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    VkImageView Spock::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
                                       uint32_t mip_levels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect_flags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mip_levels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(s_VulkanContext.Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void Spock::CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format,
                            VkSampleCountFlagBits num_samples, VkImageTiling tiling, VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_levels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = num_samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(s_VulkanContext.Device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(s_VulkanContext.Device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            FindMemoryType(s_VulkanContext.PhysicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(s_VulkanContext.Device, &allocInfo, nullptr, &image_memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(s_VulkanContext.Device, image, image_memory, 0);
    }

    void Spock::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                             VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(s_VulkanContext.Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(s_VulkanContext.Device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            FindMemoryType(s_VulkanContext.PhysicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(s_VulkanContext.Device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(s_VulkanContext.Device, buffer, buffer_memory, 0);
    }

    void Spock::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        auto command_buffer = BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(command_buffer, src, dst, 1, &copyRegion);

        EndSingleTimeCommands(command_buffer);
    }

    VkCommandBuffer Spock::BeginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = s_VulkanContext.CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(s_VulkanContext.Device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void Spock::EndSingleTimeCommands(VkCommandBuffer command_buffer) {
        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer;

        vkQueueSubmit(s_VulkanContext.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(s_VulkanContext.GraphicsQueue);

        vkFreeCommandBuffers(s_VulkanContext.Device, s_VulkanContext.CommandPool, 1, &command_buffer);
    }

    void Spock::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        auto command_buffer = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndSingleTimeCommands(command_buffer);
    }

    void Spock::TransitionImageLayout(VkImage image, VkFormat, VkImageLayout old_layout, VkImageLayout new_layout,
                                      uint32_t mip_levels) {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                   && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        EndSingleTimeCommands(commandBuffer);
    }
} // namespace spock
