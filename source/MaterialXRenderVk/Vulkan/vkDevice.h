#pragma once
#include <vulkan/vulkan.h>
#include "VkCommon.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <functional>

class VulkanSwapchain;
class VulkanRenderList;
class VulkanRenderPass;
class VulkanMaterialX;
class VulkanDeviceBuffer;
class VulkanHostBuffer;
class VulkanRenderTarget;
class VulkanTexture;

class VulkanDevice : public std::enable_shared_from_this<VulkanDevice>
{
    public:
    VulkanDevice(std::vector<const char *> requestedExtensions);
    virtual ~VulkanDevice();

    void InitializeDevice(VkSurfaceKHR windowSurface);

    VkDevice GetDevice(){ return device; }
    VkPhysicalDevice GetPhysicalDevice(){ return physicalDevice; }
    VkInstance GetInstance(){ return instance; }
    VkSurfaceKHR GetSurface(){ return surface; }
    VkQueue GetDefaultQueue(){ return queue; }
    VkCommandPool GetCommandPool(){ return commandPool; }
    VkDescriptorPool GetDescriptorPool(VkCommandBuffer commandBuffer);
    void AllocateNewDescriptorPool(VkCommandBuffer commandBuffer);
    void FreeDescriptorPools(VkCommandBuffer commandBuffer);

    std::shared_ptr<VulkanSwapchain> CreateSwapchain(const glm::uvec2 &targetExtents);
    std::shared_ptr<VulkanRenderPass> CreateRenderPass();
    std::shared_ptr<VulkanRenderList> CreateRenderList();
    std::shared_ptr<VulkanDeviceBuffer> CreateDeviceBuffer(VkBufferUsageFlags usageFlags, VkFormat _bufferFormat);
    std::shared_ptr<VulkanHostBuffer> CreateHostBuffer(VkBufferUsageFlags usageFlags, VkFormat _bufferFormat);
    std::shared_ptr<VulkanRenderTarget> CreateRenderTarget(std::vector<std::pair<VkFormat, VkImageUsageFlags>> colorFormats, bool depthTexture, glm::uvec3 targetExtents, bool multisampled=false);
    std::shared_ptr<VulkanRenderTarget> CreateRenderTarget(std::vector<std::pair<VkFormat, VkImageUsageFlags>> colorFormats, bool depthTexture, glm::uvec3 targetExtents, std::vector<VkSurfaceFormatKHR> requestedSurfaceFormats, bool multisampled=false);

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindSupportedDepthFormat();
    uint32_t FindMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);

    VkFormatProperties FindFormatProperties(VkFormat format);

    VkSampleCountFlagBits GetMaxMultisampleCount();

    void AllocateMemory(const VkMemoryRequirements &memRequirements, const VkMemoryPropertyFlags &memoryProperties, VkDeviceMemory *memory);

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    bool QuerySwapChainSupport(SwapChainSupportDetails &details, VkPhysicalDevice pd = nullptr);
    VkSurfaceFormatKHR FindSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkSurfaceFormatKHR FindSurfaceFormat();

    void AddTextureToCache(std::string textureName, std::shared_ptr<VulkanTexture> texture) { textureCache[textureName] = texture; }
    std::shared_ptr<VulkanTexture> RetrieveTextureFromCache(std::string textureName) { auto it = textureCache.find(textureName); return (it == textureCache.end() ? nullptr : it->second); }

    protected:
    void AppendValidationLayerExtension();

    void CreateInstance();
    void FindPhysicalDevice();
    void CreateDevice();
    uint32_t GetQueueFamilyIndex(VkQueueFlagBits flags);
    void CreateCommandPool();

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkCommandPool commandPool;
    std::map<VkCommandBuffer, std::vector<VkDescriptorPool>> _commandBufferDescriptorPools;
    VkSurfaceKHR surface;

    VkExtent2D windowExtent;

    std::vector<const char *> layers;
    std::vector<const char *> extensions;

    bool enableValidationLayers;
    VkDebugReportCallbackEXT debugReportCallback;

    std::map<std::string, std::shared_ptr<VulkanTexture>> textureCache;
};