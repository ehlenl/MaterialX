#include "VkDevice.h"
#include "VkTexture.h"
#include "vkRenderTarget.h"
#include "vkSwapchain.h"
#include "vkRenderList.h"
#include "vkRenderPass.h"
#include "vkMaterialX.h"
#include "vkDeviceBuffer.h"
#include "vkHostBuffer.h"

#include <vulkan/vulkan.h>

#include <stdio.h>
#include <string.h>

VulkanDevice::VulkanDevice(std::vector<const char *> requestedExtensions)
:   instance(nullptr),
    physicalDevice(nullptr),
    device(nullptr),
    queue(nullptr),
    commandPool(nullptr),
    surface(nullptr)
{
    enableValidationLayers = true;
    extensions = requestedExtensions;
    this->CreateInstance();
}

VulkanDevice::~VulkanDevice()
{
    VK_LOG << "Destroying Device" << std::endl;
    if( enableValidationLayers )
    {
        auto vkDestroyDebugReportCallbackEXTfn = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXTfn(instance, debugReportCallback, nullptr);
    }
    if( surface != nullptr )
    {
#if (__APPLE__)
#else
        vkDestroySurfaceKHR(instance, surface, nullptr);
#endif
        surface = nullptr;
    }
    for (auto& commandBufferPools : _commandBufferDescriptorPools)
    {
        for (auto& pool : commandBufferPools.second)
            vkDestroyDescriptorPool(device, pool, nullptr);
    }
    if( commandPool )
        vkDestroyCommandPool(device, commandPool, nullptr);
    if( device )
        vkDestroyDevice(device, nullptr);
    if( instance )
        vkDestroyInstance(instance, nullptr);
}

void VulkanDevice::InitializeDevice(VkSurfaceKHR windowSurface)
{
    surface = windowSurface;
    this->FindPhysicalDevice();
    this->CreateDevice();
}

std::shared_ptr<VulkanSwapchain> VulkanDevice::CreateSwapchain(const glm::uvec2 &targetExtents)
{
    return std::make_shared<VulkanSwapchain>(shared_from_this(), targetExtents);
}

std::shared_ptr<VulkanRenderPass> VulkanDevice::CreateRenderPass()
{
    return std::make_shared<VulkanRenderPass>(shared_from_this());
}

std::shared_ptr<VulkanRenderList> VulkanDevice::CreateRenderList()
{
    return std::make_shared<VulkanRenderList>(shared_from_this());
}

std::shared_ptr<VulkanDeviceBuffer> VulkanDevice::CreateDeviceBuffer(VkBufferUsageFlags usageFlags, VkFormat bufferFormat)
{
    return std::make_shared<VulkanDeviceBuffer>(shared_from_this(), usageFlags, bufferFormat);
}

std::shared_ptr<VulkanHostBuffer> VulkanDevice::CreateHostBuffer(VkBufferUsageFlags usageFlags, VkFormat bufferFormat)
{
    return std::make_shared<VulkanHostBuffer>(shared_from_this(), usageFlags, bufferFormat);
}

std::shared_ptr<VulkanRenderTarget> VulkanDevice::CreateRenderTarget(std::vector<std::pair<VkFormat, VkImageUsageFlags>> colorFormats, bool depthTexture, glm::uvec3 targetExtents, std::vector<VkSurfaceFormatKHR> requestedSurfaceFormats, bool multisampled )
{

    auto renderTarget = std::make_shared<VulkanRenderTarget>(shared_from_this(), targetExtents, requestedSurfaceFormats);

    for (auto& formatUsage : colorFormats)
    {
        auto colorTarget = std::make_shared<VulkanTexture>(shared_from_this(), formatUsage.first, targetExtents, false, multisampled);
        colorTarget->Initialize(formatUsage.second, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        colorTarget->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        renderTarget->AddColorTarget(colorTarget);
    }

    if (depthTexture)
    {
        VkFormat depthFormat = this->FindSupportedDepthFormat();
        auto depthTexture = std::make_shared<VulkanTexture>(shared_from_this(), depthFormat, targetExtents, false, multisampled);
        depthTexture->Initialize(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        depthTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        renderTarget->SetDepthTarget(depthTexture);
    }

    return renderTarget;
}

std::shared_ptr<VulkanRenderTarget> VulkanDevice::CreateRenderTarget(std::vector<std::pair<VkFormat, VkImageUsageFlags>> colorFormats, bool depthTexture, glm::uvec3 targetExtents, bool multisampled )
{
    return CreateRenderTarget(colorFormats, depthTexture, targetExtents, {}, multisampled);
}

void VulkanDevice::AppendValidationLayerExtension()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    bool foundLayer = false;
    for (VkLayerProperties prop : layerProperties)
    {
        std::cout << prop.layerName << std::endl;
        if (strcmp("VK_LAYER_KHRONOS_validation", prop.layerName) == 0)
        {
            foundLayer = true;
            break;
        }
    }
    
    if (!foundLayer)
        throw std::runtime_error("Layer VK_LAYER_KHRONOS_validation not supported\n");

    layers.push_back("VK_LAYER_KHRONOS_validation");

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionProperties.data());

    bool foundExtension = false;
    for (VkExtensionProperties prop : extensionProperties) {
        if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0) {
            foundExtension = true;
            break;
        }
    }

    if (!foundExtension)
        throw std::runtime_error("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n");
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

void VulkanDevice::CreateInstance()
{
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "vk";
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = "";
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &applicationInfo;
    
    // provide requested layers and extensions
    if( enableValidationLayers )
        AppendValidationLayerExtension();

    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VK_ERROR_CHECK( vkCreateInstance(&createInfo, NULL, &instance) );

    if (enableValidationLayers)
    {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        createInfo.pfnCallback = &debugReportCallbackFn;

        auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        if (vkCreateDebugReportCallbackEXT == nullptr) {
            throw std::runtime_error("Could not load vkCreateDebugReportCallbackEXT");
        }

        VK_ERROR_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, NULL, &debugReportCallback));
    }
}

void VulkanDevice::FindPhysicalDevice()
{
    uint32_t deviceCount;
    VK_ERROR_CHECK( vkEnumeratePhysicalDevices(instance, &deviceCount, NULL) );
    if (deviceCount == 0)
        throw std::runtime_error("Could not find a device with vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::vector<VkPhysicalDevice> supportedDevices;

    for (VkPhysicalDevice device : devices)
    {
        SwapChainSupportDetails swapChainSupport;
        if( this->QuerySwapChainSupport(swapChainSupport, device) )
        {
            if( !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty() )
            {
                physicalDevice = device;
                supportedDevices.push_back(device);
            }
        }
        else
        {
            physicalDevice = device;
            break;
        }
    }

    // enumerate supported devices
    for (size_t i=0; i<supportedDevices.size(); ++i)
    {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(supportedDevices[i], &deviceProps);
        std::cout << i+1 << ". " << deviceProps.deviceName << std::endl;
    }

    physicalDevice = supportedDevices[0];

    if (supportedDevices.size() > 1)
    {
        // allow selection of supported device
        int deviceNum;
        std::cout << "Select device: ";
        std::cin >> deviceNum;
        if (deviceNum > 0 && deviceNum <= supportedDevices.size())
            physicalDevice = supportedDevices[deviceNum - 1];
    }

    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

    VK_LOG << "API: " << VK_VERSION_MAJOR(deviceProps.apiVersion) << "." << VK_VERSION_MINOR(deviceProps.apiVersion) << "." << VK_VERSION_PATCH(deviceProps.apiVersion) << std::endl;
    VK_LOG << "Driver: " << VK_VERSION_MAJOR(deviceProps.driverVersion) << "." << VK_VERSION_MINOR(deviceProps.driverVersion) << "." << VK_VERSION_PATCH(deviceProps.driverVersion) << std::endl;
    VK_LOG << "Device: " << deviceProps.deviceName << std::endl;
}

void VulkanDevice::CreateDevice()
{
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };
#if defined(__APPLE__)
    deviceExtensions.push_back("VK_KHR_portability_subset");
#endif

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = this->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
    queueCreateInfo.queueCount = 1;
    float queuePriorities = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());  // need to specify validation layers here as well
    deviceCreateInfo.ppEnabledLayerNames = layers.data();
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // enabling features
    VkPhysicalDeviceFeatures2 physicalFeatures2 = {};
    physicalFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalFeatures2);
    deviceCreateInfo.pNext = &physicalFeatures2;
    
    VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Feature = {};
    indexTypeUint8Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
    physicalFeatures2.pNext = &indexTypeUint8Feature;

    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.bufferDeviceAddress = true;
    indexTypeUint8Feature.pNext = &vulkan12Features;

    VK_ERROR_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device));

    // Get the device queue
    vkGetDeviceQueue(device, queueCreateInfo.queueFamilyIndex, 0, &queue);

    this->CreateCommandPool();
}

void VulkanDevice::CreateCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = this->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);

    VK_ERROR_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool));
}

VkDescriptorPool VulkanDevice::GetDescriptorPool(VkCommandBuffer commandBuffer)
{
    if (_commandBufferDescriptorPools.find(commandBuffer) == _commandBufferDescriptorPools.end())
        AllocateNewDescriptorPool(commandBuffer);
    if (_commandBufferDescriptorPools[commandBuffer].size() == 0)
        AllocateNewDescriptorPool(commandBuffer);

    return *(_commandBufferDescriptorPools[commandBuffer].rbegin());
}

void VulkanDevice::AllocateNewDescriptorPool(VkCommandBuffer commandBuffer)
{
    auto& pools = _commandBufferDescriptorPools[commandBuffer];
    pools.resize(pools.size() + 1);

    // default pool sizes
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256 },
        { VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256 },
        { VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 },
        { VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 256 },
        { VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256 }
    };

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 256;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VK_ERROR_CHECK(
        vkCreateDescriptorPool(this->device, &poolInfo, NULL, &pools[pools.size() - 1]));
}

// Call this when you're done with the command buffer
void VulkanDevice::FreeDescriptorPools(VkCommandBuffer commandBuffer)
{
    auto& pools = _commandBufferDescriptorPools[commandBuffer];
    for (auto& pool : pools)
        vkDestroyDescriptorPool(this->device, pool, nullptr);
    pools.clear();
}

uint32_t VulkanDevice::GetQueueFamilyIndex(VkQueueFlagBits flags)
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for(; i<queueFamilies.size(); ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if( props.queueCount > 0 && (props.queueFlags & flags) )
        {
            if( surface != nullptr )
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if( presentSupport )
                    break;
            }else
                break;
        }
    }

    if( i == queueFamilies.size() )
        throw std::runtime_error("Could not find a queue family that supports graphics.");

    return i;
}

bool VulkanDevice::QuerySwapChainSupport(SwapChainSupportDetails &details, VkPhysicalDevice pd)
{
    if( pd == nullptr )
        pd = physicalDevice;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &details.capabilities);

    // formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &formatCount, details.formats.data());
    }

    // present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, details.presentModes.data());
    }
    return true;
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    return VK_FORMAT_UNDEFINED;
}

VkSurfaceFormatKHR VulkanDevice::FindSurfaceFormat()
{
    SwapChainSupportDetails swapChainSupport;
    this->QuerySwapChainSupport(swapChainSupport);
    return this->FindSurfaceFormat(swapChainSupport.formats);
}

VkSurfaceFormatKHR VulkanDevice::FindSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
            if( availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
                return availableFormat;
    }
    return availableFormats[0];
}

VkFormat VulkanDevice::FindSupportedDepthFormat()
{
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

uint32_t VulkanDevice::FindMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }

    throw std::runtime_error("Appropriate memory type not found.");
}

void VulkanDevice::AllocateMemory(const VkMemoryRequirements &memRequirements, const VkMemoryPropertyFlags &memoryProperties, VkDeviceMemory *memory)
{
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = this->FindMemoryType(memRequirements.memoryTypeBits, memoryProperties);

    if (vkAllocateMemory(this->GetDevice(), &allocInfo, nullptr, memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate memory for image.");
}

VkFormatProperties VulkanDevice::FindFormatProperties(VkFormat format)
{
    VkFormatProperties props = {};
    vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &props);
    return props;
}

VkSampleCountFlagBits VulkanDevice::GetMaxMultisampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags sampleCounts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (sampleCounts & VK_SAMPLE_COUNT_64_BIT)
        return VK_SAMPLE_COUNT_64_BIT;
    if (sampleCounts & VK_SAMPLE_COUNT_32_BIT)
        return VK_SAMPLE_COUNT_32_BIT;
    if (sampleCounts & VK_SAMPLE_COUNT_16_BIT)
        return VK_SAMPLE_COUNT_16_BIT;
    if (sampleCounts & VK_SAMPLE_COUNT_8_BIT)
        return VK_SAMPLE_COUNT_8_BIT;
    if (sampleCounts & VK_SAMPLE_COUNT_4_BIT)
        return VK_SAMPLE_COUNT_4_BIT;
    if (sampleCounts & VK_SAMPLE_COUNT_2_BIT)
        return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}
