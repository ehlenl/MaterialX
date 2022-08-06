#pragma once
#include "VkDevice.h"
#include <glm/glm.hpp>

class VulkanBuffer;

class VulkanTexture
{
    public:
    VulkanTexture(VulkanDevicePtr device, VkFormat textureFormat, const glm::uvec3 &textureDimensions, bool generateMipmaps, bool multisampled = false);
    virtual ~VulkanTexture();

    virtual void Initialize();
    virtual void Initialize(VkImageUsageFlags usage, VkMemoryPropertyFlags memProperties, VkImageAspectFlags aspectFlags);
    virtual void Initialize(VkImageAspectFlags aspectFlags, VkImage acquiredImage);
    virtual void TransitionImageLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    virtual void TransitionImageLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange, VkCommandBuffer commandBuffer);
    virtual void TransitionImageLayout(VkImage transitionImage, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange, VkCommandBuffer commandBuffer);
    virtual VkImage Resolve(VkCommandBuffer commandBuffer);

    void CopyBufferToTexture(std::shared_ptr<VulkanBuffer> pixelData);
    void WriteToFile(std::string filename);

    VkImageView GetImageView(){ return imageView; }
    VkImage GetImage(){ return image; }

    glm::uvec3 GetDimensions() { return dimensions; }

    VkSampleCountFlagBits SampleCount() { return sampleCount; }
    VkFormat TextureFormat() { return imageFormat; }

    protected:
    void CreateImage(VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    void CreateImageView(VkImageAspectFlags aspectFlags);
    void GenerateMipmaps();

    glm::uvec3 dimensions;
    uint32_t mipLevels;

    VkSampleCountFlagBits sampleCount;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;

    //
    VkImage resolvedImage;
    VkDeviceMemory resolvedImageMemory;

    VkFormat imageFormat;

    bool destroyImage;
    bool generateMipmaps;

    VulkanDevicePtr device;
};
