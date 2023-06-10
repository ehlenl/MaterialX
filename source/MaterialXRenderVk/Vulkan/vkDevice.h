#pragma once
#include <vulkan/vulkan.hpp>

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

/// Vulkan Device shared pointer
using VulkanDevicePtr = std::shared_ptr<class VulkanDevice>;

class VulkanDevice : public std::enable_shared_from_this<VulkanDevice>
{
    public:
    /// Create a new context
    static VulkanDevicePtr create()
    {
        std::vector<const char*> vkExtensions = {
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        return VulkanDevicePtr(new VulkanDevice(vkExtensions));
    }

    
    virtual ~VulkanDevice();

    void InitializeDevice(vk::SurfaceKHR windowSurface);

    VkDevice GetDevice(){ return device; }
    VkPhysicalDevice GetPhysicalDevice(){ return physicalDevice; }
    VkInstance GetInstance(){ return instance; }
    vk::Instance GetInstanceCPP() { return _instance; }
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
    
    void CreateCommandPool();
    void CreateCommandPoolCPP();

    protected:
    VulkanDevice(std::vector<const char*> requestedExtensions);
    void AppendValidationLayerExtension();
    void CreateInstance();
    void FindPhysicalDevice();
    void CreateDevice();

    void AppendValidationLayerExtensionCPP();
    void CreateInstanceCPP();
    void FindPhysicalDeviceCPP();
    void CreateDeviceCPP();
    
    
    
    uint32_t GetQueueFamilyIndex(VkQueueFlagBits flags);
    

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _queue;
    vk::CommandPool _commandPool;
    std::pair<uint32_t, uint32_t> _graphicsandPresentQueueIndex;  

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