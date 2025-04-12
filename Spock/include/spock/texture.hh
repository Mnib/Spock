#pragma once

#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace spock
{
    class Texture2D {
      public:
        Texture2D(uint8_t *data, int width, int height, int channels, int mip_levels);
        Texture2D(const Texture2D &) = delete;
        Texture2D operator=(const Texture2D &) = delete;
        ~Texture2D();

        // Opens an image file from disk and create a GPU texture from it.
        static std::shared_ptr<Texture2D> FromFile(const std::string &path);

        // Getters
        VkImageView GetImageView() const {
            return m_TextureImageView;
        }

        VkSampler GetSampler() const {
            return m_TextureSampler;
        }

      private:
        void GenerateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mip_levels);
        void CreateTextureImageView();
        void CreateTextureSampler();

      private:
        int m_Width;
        int m_Height;
        int m_Channels;
        int m_MipLevels;
        VkImage m_TextureImage;
        VkDeviceMemory m_TextureImageMemory;
        VkImageView m_TextureImageView;
        VkSampler m_TextureSampler;
    };
} // namespace spock
