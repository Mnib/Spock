#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

#include "spock/spock.hh"
#include "spock/texture.hh"
#include "spock/vulkan.hh"

namespace spock
{
    std::shared_ptr<Texture2D> Texture2D::FromFile(const std::string &path) {
        int width, height, channels;
        stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        auto mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        // Create the texture
        auto texture = std::make_shared<Texture2D>(pixels, width, height, STBI_rgb_alpha, mip_levels);

        // Free the pixels
        stbi_image_free(pixels);

        return texture;
    }

    Texture2D::Texture2D(uint8_t *data, int width, int height, int channels, int mip_levels)
        : m_Width(width)
        , m_Height(height)
        , m_Channels(channels)
        , m_MipLevels(mip_levels)
        , m_TextureImage(nullptr)
        , m_TextureImageMemory(nullptr)
        , m_TextureImageView(nullptr)
        , m_TextureSampler(nullptr) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        auto image_size = width * height * channels;

        Spock::CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                            stagingBufferMemory);

        void *staging_data;
        vkMapMemory(s_VulkanContext.Device, stagingBufferMemory, 0, image_size, 0, &staging_data);
        memcpy(staging_data, data, static_cast<size_t>(image_size));
        vkUnmapMemory(s_VulkanContext.Device, stagingBufferMemory);

        Spock::CreateImage(
            width, height, mip_levels, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

        Spock::TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
        Spock::CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height));
        // Transitionned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL in `GenerateMipmaps`.

        vkDestroyBuffer(s_VulkanContext.Device, stagingBuffer, nullptr);
        vkFreeMemory(s_VulkanContext.Device, stagingBufferMemory, nullptr);

        GenerateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, m_MipLevels);
        CreateTextureImageView();
        CreateTextureSampler();
    }

    static VkFormatProperties GetPhysicalDeviceFormatProperties(VkFormat format) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(s_VulkanContext.PhysicalDevice, format, &props);
        return props;
    }

    void Texture2D::GenerateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height,
                                    uint32_t mip_levels) {
        VkFormatProperties formatProperties = GetPhysicalDeviceFormatProperties(format);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = Spock::BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = width;
        int32_t mipHeight = height;

        for (uint32_t i = 1; i < mip_levels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        Spock::EndSingleTimeCommands(commandBuffer);
    }

    void Texture2D::CreateTextureImageView() {
        m_TextureImageView =
            Spock::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
    }

    void Texture2D::CreateTextureSampler() {
        VkPhysicalDeviceProperties properties = s_VulkanContext.PhysicalDeviceProperties;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);

        if (vkCreateSampler(s_VulkanContext.Device, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    Texture2D::~Texture2D() {
        vkDestroySampler(s_VulkanContext.Device, m_TextureSampler, nullptr);
        vkDestroyImageView(s_VulkanContext.Device, m_TextureImageView, nullptr);
        vkDestroyImage(s_VulkanContext.Device, m_TextureImage, nullptr);
        vkFreeMemory(s_VulkanContext.Device, m_TextureImageMemory, nullptr);
    }
} // namespace spock
