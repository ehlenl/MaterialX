#pragma once
#include "vkDevice.h"

class VulkanSwapchain;
class VulkanRenderTarget;

class VulkanRenderPass
{
    public:
    VulkanRenderPass(VulkanDevicePtr _device);
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

    VulkanDevicePtr device;

    VkRenderPass renderPass;
    VkCommandBuffer commandBuffer;

    VkFence fence;
};
