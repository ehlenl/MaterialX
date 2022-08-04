#pragma once
#include "vkDevice.h"

class VulkanSwapchain;
class VulkanRenderTarget;

class VulkanRenderPass
{
    public:
    VulkanRenderPass(std::shared_ptr<VulkanDevice> _device);
    virtual ~VulkanRenderPass();

    VkRenderPass GetRenderPass(){ return renderPass; }
    void Initialize(std::shared_ptr<VulkanRenderTarget> renderTarget, bool createRenderPass = true);
    std::shared_ptr<VulkanRenderTarget> GetRenderTarget() { return renderTarget; }

    VkCommandBuffer GetCommandBuffer(){ return commandBuffer; }

    void CreateCommandBuffers();
    void Draw();

    void BeginRenderPass(uint32_t targetIndex);
    void EndRenderPass();

    protected:
    void CreateRenderPass();

    std::shared_ptr<VulkanRenderTarget> renderTarget;

    std::shared_ptr<VulkanDevice> device;

    VkRenderPass renderPass;
    VkCommandBuffer commandBuffer;

    VkFence fence;
};
