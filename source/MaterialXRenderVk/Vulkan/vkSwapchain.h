#include "vkDevice.h"
#include <atomic>

class VulkanRenderPass;
class VulkanTexture;
class VulkanRenderTarget;

class VulkanSwapchain
{
    public:
    VulkanSwapchain(std::shared_ptr<VulkanDevice> device, const glm::uvec2 &extent);
    virtual ~VulkanSwapchain();

    glm::uvec2 GetExtent();
    VkFormat GetImageFormat(){ return swapchainImageFormat; }
    uint32_t GetNumberOfImages(){ return imageCount; }

    std::shared_ptr<VulkanTexture> GetCurrentImage();
    std::shared_ptr<VulkanRenderTarget> GetRenderTarget() { return renderTarget; }

    void Bind(std::shared_ptr<VulkanTexture> renderTarget);

    void Blit();

    protected:
    void CreateSwapchain();
    void CreateSemaphores();
    void CreateCommandBuffers();

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    std::atomic<int> swapchainMutex;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::shared_ptr<VulkanRenderTarget> renderTarget;

    std::vector<VkCommandBuffer> blitCommandBuffers;

    std::shared_ptr<VulkanDevice> device;
    uint32_t imageCount;
    uint32_t currentFrame;
    bool blit;
};
