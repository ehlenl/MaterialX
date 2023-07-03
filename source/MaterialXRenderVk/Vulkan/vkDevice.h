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

    //REQUIRED Methods:
    /// InitializeDevice creates a swapchain for now, todo move it out 
    void InitializeDevice(vk::SurfaceKHR windowSurface, uint32_t width, uint32_t height);
    vk::Instance GetInstance() { return _instance; }
    vk::Device GetDevice() { return _device; }
    vk::PhysicalDevice GetPhysicalDevice() { return _physicalDevice; }
    void CreateCommandPool();
    uint32_t FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties);
    vk::SurfaceKHR GetSurface() { return _surface; }

    struct SwapChainData
    {
        SwapChainData(vk::PhysicalDevice const& physicalDevice,
                      vk::Device const& device,
                      vk::SurfaceKHR const& surface,
                      vk::Extent2D const& extent,
                      vk::ImageUsageFlags usage,
                      vk::SwapchainKHR const& oldSwapChain,
                      uint32_t graphicsFamilyIndex,
                      uint32_t presentFamilyIndex);
        SwapChainData(){};

        void clear(vk::Device const& device);
        vk::Format _colorFormat;
        vk::SwapchainKHR _swapChain;
        std::vector<vk::Image> _images;
        std::vector<vk::ImageView> _imageViews;
    };


    VkSurfaceKHR GetSurface_C(){ return surface_C; }

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

    VkFormatProperties FindFormatProperties(VkFormat format);

    VkSampleCountFlagBits GetMaxMultisampleCount();

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
    
    
    // HELPER TEST
    vk::Semaphore _imageAvailableSemaphore;
    vk::Semaphore _renderingFinishedSemaphore;
    void clearScreen();

    protected:
    VulkanDevice(std::vector<const char*> requestedExtensions);
    
    // REQUIRED METHODS
    void AppendValidationLayerExtension();
    void CreateInstance();
    void FindPhysicalDevice();
    void CreateDevice();
    
    uint32_t GetQueueFamilyIndex(VkQueueFlagBits flags);
    

    VkInstance instance_C;
    VkPhysicalDevice physicalDevice_C;
    VkDevice device_C;
    VkQueue queue_C;
    VkCommandPool commandPool_C;

    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _queue;
    vk::CommandPool _commandPool;
    vk::CommandBuffer _commandBuffer;
    std::pair<uint32_t, uint32_t> _graphicsandPresentQueueIndex;

    
    // set by renderer on InitializeDevice
    vk::SurfaceKHR _surface;
    SwapChainData _swapChainData;
    vk::Extent2D _surfaceExtent;

    std::map<VkCommandBuffer, std::vector<VkDescriptorPool>> _commandBufferDescriptorPools;
    VkSurfaceKHR surface_C;
    
    VkExtent2D windowExtent;

    std::vector<const char *> layers;
    std::vector<const char *> extensions;

    bool enableValidationLayers;
    VkDebugReportCallbackEXT debugReportCallback;

    std::map<std::string, std::shared_ptr<VulkanTexture>> textureCache;
};