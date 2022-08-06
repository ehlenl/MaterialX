#pragma once
#include "vkDevice.h"

class VulkanTexture;

class VulkanRenderTarget
{
    public:
    VulkanRenderTarget(VulkanDevicePtr device, glm::uvec3 extent);
    VulkanRenderTarget(VulkanDevicePtr device, glm::uvec3 extent, std::vector<VkSurfaceFormatKHR> requestedSurfaceFormat);
    virtual ~VulkanRenderTarget();
    
    void CreateFrameBuffer(VkRenderPass renderPass);

    void AddColorTarget(std::shared_ptr<VulkanTexture> colorTarget);
    void SetDepthTarget(std::shared_ptr<VulkanTexture> depthTarget);

    uint32_t GetColorTargetCount() { return static_cast<uint32_t>(colorTextures.size()); }
    std::vector<std::shared_ptr<VulkanTexture>> &GetColorTargets() { return colorTextures; }
    std::shared_ptr<VulkanTexture> GetColorTarget(uint32_t index) { return colorTextures[index]; }
    std::shared_ptr<VulkanTexture> GetDepthTarget() { return depthTexture; }

    VkAttachmentDescription GetColorAttachmentDescription() { return colorAttachmentDescription; }
    VkAttachmentDescription GetDepthAttachmentDescription() { return depthAttachmentDescription; }

    const glm::uvec3 &GetExtent() { return extent; }
    VkFramebuffer GetFramebuffer(uint32_t index) { return frameBuffers[index]; }

    protected:
    VulkanDevicePtr device;

    std::vector<std::shared_ptr<VulkanTexture>> colorTextures;
    VkAttachmentDescription colorAttachmentDescription;
    std::shared_ptr<VulkanTexture> depthTexture;
    VkAttachmentDescription depthAttachmentDescription;

    std::vector<VkSurfaceFormatKHR> requestedSurfaceFormats;

    std::vector<VkFramebuffer> frameBuffers;

    glm::uvec3 extent;
};
