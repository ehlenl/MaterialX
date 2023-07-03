#include "VkDevice.h"
#if 0
#include "VkTexture.h"
#include "vkRenderTarget.h"
#include "vkSwapchain.h"
#include "vkRenderList.h"
#include "vkRenderPass.h"
#include "vkMaterialX.h"
#include "vkDeviceBuffer.h"
#include "vkHostBuffer.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <numeric>
#include <iterator>
#include <algorithm> 
#include <cstdint>
#include <iostream>
#include <iterator>
#include <chrono>
#include <thread>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VulkanDevice::VulkanDevice(std::vector<const char *> requestedExtensions)
:   instance_C(nullptr),
    physicalDevice_C(nullptr),
    device_C(nullptr),
    queue_C(nullptr),
    commandPool_C(nullptr),
    surface_C(nullptr)
{
    enableValidationLayers = true;
    extensions = requestedExtensions;
    this->CreateInstance();
}

// FIXME: USE C++ types
VulkanDevice::~VulkanDevice()
{
    VK_LOG << "Destroying Device" << std::endl;
    if( enableValidationLayers )
    {
        auto vkDestroyDebugReportCallbackEXTfn = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance_C, "vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXTfn(instance_C, debugReportCallback, nullptr);
    }
    if( surface_C != nullptr )
    {
#if (__APPLE__)
#else
        vkDestroySurfaceKHR(instance_C, surface_C, nullptr);
#endif
        surface_C = nullptr;
    }
    for (auto& commandBufferPools : _commandBufferDescriptorPools)
    {
        for (auto& pool : commandBufferPools.second)
            vkDestroyDescriptorPool(device_C, pool, nullptr);
    }
    if( commandPool_C )
        vkDestroyCommandPool(device_C, commandPool_C, nullptr);
    if( device_C )
        vkDestroyDevice(device_C, nullptr);
    if( instance_C )
        vkDestroyInstance(instance_C, nullptr);
}

void VulkanDevice::InitializeDevice(vk::SurfaceKHR windowSurface, uint32_t width, uint32_t height)
{
    surface_C = windowSurface;
    _surface = windowSurface;
    FindPhysicalDevice();
    CreateDevice();

    // Create a swapchain for testing purpose
    _surfaceExtent = vk::Extent2D(width, height);
    _swapChainData = SwapChainData(_physicalDevice,
                                   _device,
                                   _surface,
                                   _surfaceExtent,
                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                   {},
                                   _graphicsandPresentQueueIndex.first,
                                   _graphicsandPresentQueueIndex.second);
    

}

void VulkanDevice::AppendValidationLayerExtension()
{
    bool foundLayer = false;
    for (const vk::LayerProperties& prop : vk::enumerateInstanceLayerProperties())
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

    bool foundExtension = false;
    for (const vk::ExtensionProperties& prop : vk::enumerateInstanceExtensionProperties())
    {
        if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0)
        {
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

    static vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);


    vk::ApplicationInfo applicationInfo("MaterialX", 1, "MaterialXRender", 1, VK_API_VERSION_1_2);

    // provide requested layers and extensions
    if (enableValidationLayers)
    {
        AppendValidationLayerExtension();
    }

    _instance = vk::createInstance(vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &applicationInfo, layers, extensions));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);
}

void VulkanDevice::FindPhysicalDevice()
{
    _physicalDevice = _instance.enumeratePhysicalDevices().front();
    
    uint32_t apiVersion = _physicalDevice.getProperties().apiVersion;
    uint32_t driverVersion = _physicalDevice.getProperties().driverVersion;

    VK_LOG << "API: " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
    VK_LOG << "Driver: " << VK_VERSION_MAJOR(driverVersion) << "." << VK_VERSION_MINOR(driverVersion) << "." << VK_VERSION_PATCH(driverVersion) << std::endl;
    VK_LOG << "Device: " << _physicalDevice.getProperties().deviceName << std::endl;
}

void VulkanDevice::CreateDevice()
{

        // Find graphics queue
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = _physicalDevice.getQueueFamilyProperties();
    assert(queueFamilyProperties.size() < std::numeric_limits<uint32_t>::max());


    // get the first index into queueFamiliyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(),
                                                    queueFamilyProperties.end(),
                                                    [](vk::QueueFamilyProperties const& qfp)
                                                    {
        return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
    });
    assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());
    uint32_t graphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // determine a queueFamilyIndex that supports present
    // first check if the graphicsQueueFamiliyIndex is good enough
    size_t presentQueueFamilyIndex = _physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(graphicsQueueFamilyIndex), _surface)
                                         ? graphicsQueueFamilyIndex
                                         : queueFamilyProperties.size();
    if (presentQueueFamilyIndex == queueFamilyProperties.size())
    {
        // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                _physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), _surface))
            {
                graphicsQueueFamilyIndex = static_cast<uint32_t>(i);
                presentQueueFamilyIndex = i;
                break;
            }
        }
        if (presentQueueFamilyIndex == queueFamilyProperties.size())
        {
            // there's nothing like a single family index that supports both graphics and present -> look for an other
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if (_physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), _surface))
                {
                    presentQueueFamilyIndex = i;
                    break;
                }
            }
        }
    }
    if ((graphicsQueueFamilyIndex == queueFamilyProperties.size()) || (presentQueueFamilyIndex == queueFamilyProperties.size()))
    {
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    _graphicsandPresentQueueIndex = std::make_pair(graphicsQueueFamilyIndex, presentQueueFamilyIndex);

    #if 0
    // DEBUG
    // get the first index into queueFamiliyProperties which supports graphics
    const auto& graphicsQueueFamilyProperty =
        std::find_if(queueFamilyProperties.begin(),
                     queueFamilyProperties.end(),
                     [](vk::QueueFamilyProperties const& qfp)
                     {
        return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
        });
    
    assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());

    uint32_t graphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    
    if (_physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, _surface))
    {
        _graphicsandPresentQueueIndex = std::make_pair(graphicsQueueFamilyIndex, graphicsQueueFamilyIndex);
        // does the first graphicsQueueFamilyIndex also support presents
    }

    // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
    // graphics and present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            _physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), _surface))
        {
            _graphicsandPresentQueueIndex = std::make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));
        }
    }

    // there's nothing like a single family index that supports both graphics and present -> look for an other family
    // index that supports present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (_physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), _surface))
        {
            _graphicsandPresentQueueIndex = std::make_pair(graphicsQueueFamilyIndex, static_cast<uint32_t>(i));
        }
    }
    #endif
    ///  Create device and queue

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };


    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, _graphicsandPresentQueueIndex.first, 1, &queuePriority);
    vk::DeviceCreateInfo deviceCreateInfo({}, deviceQueueCreateInfo, {}, deviceExtensions);

    _device = _physicalDevice.createDevice(deviceCreateInfo);

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    // initialize function pointers for instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);
#endif

    _queue = _device.getQueue(_graphicsandPresentQueueIndex.first, 0);
}

void VulkanDevice::CreateCommandPool()
{
    vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _graphicsandPresentQueueIndex.first);
    _commandPool = _device.createCommandPool(commandPoolCreateInfo);

    _commandBuffer = _device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(_commandPool, vk::CommandBufferLevel::ePrimary, 1)).front();
    
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
        vkCreateDescriptorPool(this->device_C, &poolInfo, NULL, &pools[pools.size() - 1]));
}

// Call this when you're done with the command buffer
void VulkanDevice::FreeDescriptorPools(VkCommandBuffer commandBuffer)
{
    auto& pools = _commandBufferDescriptorPools[commandBuffer];
    for (auto& pool : pools)
        vkDestroyDescriptorPool(this->device_C, pool, nullptr);
    pools.clear();
}

uint32_t VulkanDevice::GetQueueFamilyIndex(VkQueueFlagBits flags)
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_C, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_C, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for(; i<queueFamilies.size(); ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if( props.queueCount > 0 && (props.queueFlags & flags) )
        {
            if( surface_C != nullptr )
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_C, i, surface_C, &presentSupport);
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
        pd = physicalDevice_C;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface_C, &details.capabilities);

    // formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface_C, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface_C, &formatCount, details.formats.data());
    }

    // present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface_C, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface_C, &presentModeCount, details.presentModes.data());
    }
    return true;
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


void VulkanDevice::clearScreen()
{
    // Create a RenderPass
        std::array<vk::AttachmentDescription, 1> attachmentDescriptions;
        attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
                                                              _swapChainData._colorFormat,
                                                              vk::SampleCountFlagBits::e1,
                                                              vk::AttachmentLoadOp::eClear,
                                                              vk::AttachmentStoreOp::eStore,
                                                              vk::AttachmentLoadOp::eDontCare,
                                                              vk::AttachmentStoreOp::eDontCare,
                                                              vk::ImageLayout::eUndefined,
                                                              vk::ImageLayout::ePresentSrcKHR);

        /*attachmentDescriptions[1] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
                                                              depthFormat,
                                                              vk::SampleCountFlagBits::e1,
                                                              vk::AttachmentLoadOp::eClear,
                                                              vk::AttachmentStoreOp::eDontCare,
                                                              vk::AttachmentLoadOp::eDontCare,
                                                              vk::AttachmentStoreOp::eDontCare,
                                                              vk::ImageLayout::eUndefined,
                                                              vk::ImageLayout::eDepthStencilAttachmentOptimal);*/

        vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
        //vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        //vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);
        vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, nullptr);

        vk::RenderPass renderPass = _device.createRenderPass(vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpass));

        // Create a FrameBuffer(s)
        std::vector<vk::Framebuffer> framebuffers;
        framebuffers.reserve(_swapChainData._imageViews.size());
        vk::ImageView attachments[2];
        vk::FramebufferCreateInfo framebufferCreateInfo(vk::FramebufferCreateFlags(), renderPass, 1, attachments, _surfaceExtent.width, _surfaceExtent.height, 1);

        for (auto const& view : _swapChainData._imageViews)
        {
            attachments[0] = view;
            framebuffers.push_back(_device.createFramebuffer(framebufferCreateInfo));
        }

        // Build Command Buffer to clear
        const uint64_t FenceTimeout = 100000000;

        vk::Semaphore imageAcquiredSemaphore = _device.createSemaphore(vk::SemaphoreCreateInfo());
        vk::ResultValue<uint32_t> currentBuffer = _device.acquireNextImageKHR(_swapChainData._swapChain, FenceTimeout, imageAcquiredSemaphore, nullptr);
        assert(currentBuffer.result == vk::Result::eSuccess);
        assert(currentBuffer.value < framebuffers.size());
        std::cout << "acquired image" << std::endl;

        _commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color = vk::ClearColorValue(1.0f, 0.0f, 1.0f, 0.2f);
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderPassBeginInfo renderPassBeginInfo(renderPass, framebuffers[currentBuffer.value], vk::Rect2D(vk::Offset2D(0, 0), _surfaceExtent), clearValues);

        _commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        _commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(_surfaceExtent.width), static_cast<float>(_surfaceExtent.height), 0.0f, 1.0f));
        _commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _surfaceExtent));
        _commandBuffer.endRenderPass();
        _commandBuffer.end();

        vk::Fence drawFence = _device.createFence(vk::FenceCreateInfo());

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo(imageAcquiredSemaphore, waitDestinationStageMask, _commandBuffer);
        
        vk::Queue graphicsQueue = _device.getQueue(_graphicsandPresentQueueIndex.first, 0);
        vk::Queue presentQueue = _device.getQueue(_graphicsandPresentQueueIndex.second, 0);
        graphicsQueue.submit(submitInfo, drawFence);

        while (vk::Result::eTimeout == _device.waitForFences(drawFence, VK_TRUE, FenceTimeout));

        vk::Result result = presentQueue.presentKHR(vk::PresentInfoKHR({}, _swapChainData._swapChain, currentBuffer.value));
        switch (result)
        {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                assert(false); // an unexpected result is returned !
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        _device.waitIdle();

        _device.destroyFence(drawFence);
        _device.destroySemaphore(imageAcquiredSemaphore);
        _device.destroyRenderPass(renderPass);


#if 0
    // Acquire image
    const uint64_t FenceTimeout = 100000000;
    
    vk::Semaphore imageAcquiredSemaphore = _device.createSemaphore(vk::SemaphoreCreateInfo());
    vk::ResultValue<uint32_t> currentBuffer = _device.acquireNextImageKHR(_swapChainData._swapChain, FenceTimeout, imageAcquiredSemaphore, nullptr);

    assert(currentBuffer.result == vk::Result::eSuccess);
    //assert(currentBuffer.value < framebuffers.size());
    std::cout << "acquired image" << std::endl;

    // Wait for image to be available and draw
    vk::Semaphore imageAcquiredSemaphore = _device.createSemaphore(vk::SemaphoreCreateInfo());
    
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(imageAcquiredSemaphore, waitDestinationStageMask, _commandBuffer);
    
    //VkSubmitInfo submitInfo = {};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //submitInfo.waitSemaphoreCount = 1;
    //submitInfo.pWaitSemaphores = &imageAvailableSemaphore;

    //submitInfo.signalSemaphoreCount = 1;
    //submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    // This is the stage where the queue should wait on the semaphore (it doesn't have to wait with drawing, for example)
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &presentCommandBuffers[imageIndex];

    if (vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        std::cerr << "failed to submit draw command buffer" << std::endl;
        exit(1);
    }

    std::cout << "submitted draw command buffer" << std::endl;

    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (res != VK_SUCCESS)
    {
        std::cerr << "failed to submit present command buffer" << std::endl;
        exit(1);
    }

    std::cout << "submitted presentation command buffer" << std::endl;
#endif
}

uint32_t VulkanDevice::FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = _physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((memoryTypeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }

    throw std::runtime_error("Appropriate memory type not found.");
}

VkFormatProperties VulkanDevice::FindFormatProperties(VkFormat format)
{
    VkFormatProperties props = {};
    vkGetPhysicalDeviceFormatProperties(this->physicalDevice_C, format, &props);
    return props;
}

VkSampleCountFlagBits VulkanDevice::GetMaxMultisampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice_C, &physicalDeviceProperties);

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


VulkanDevice::SwapChainData::SwapChainData(vk::PhysicalDevice const& physicalDevice,
                             vk::Device const& device,
                             vk::SurfaceKHR const& surface,
                             vk::Extent2D const& extent,
                             vk::ImageUsageFlags usage,
                             vk::SwapchainKHR const& oldSwapChain,
                             uint32_t graphicsQueueFamilyIndex,
                             uint32_t presentQueueFamilyIndex)
{
    // 
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
    assert(!formats.empty());

    vk::SurfaceFormatKHR surfaceFormat = formats[0];

    if (formats.size() == 1)
    {
        if (formats[0].format == vk::Format::eUndefined)
        {
            surfaceFormat.format = vk::Format::eB8G8R8A8Unorm;
            surfaceFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    }
    else
    {
        // request several formats, the first found will be used
        vk::Format requestedFormats[] = { vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm };
        vk::ColorSpaceKHR requestedColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        for (size_t i = 0; i < sizeof(requestedFormats) / sizeof(requestedFormats[0]); i++)
        {
            vk::Format requestedFormat = requestedFormats[i];
            auto it = std::find_if(formats.begin(),
                                   formats.end(),
                                   [requestedFormat, requestedColorSpace](vk::SurfaceFormatKHR const& f)
                                   {
                return (f.format == requestedFormat) && (f.colorSpace == requestedColorSpace);
            });
            if (it != formats.end())
            {
                surfaceFormat = *it;
                break;
            }
        }
    }
    assert(surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);

    _colorFormat = surfaceFormat.format;

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    vk::Extent2D swapchainExtent;
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        // If the surface size is undefined, the size is set to the size of the images requested.
        swapchainExtent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfaceCapabilities.currentExtent;
    }
    vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
                                                       ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                       : surfaceCapabilities.currentTransform;
    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
        (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
        : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
        : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)        ? vk::CompositeAlphaFlagBitsKHR::eInherit
                                                                                                         : vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vk::PresentModeKHR presentationMode = vk::PresentModeKHR::eFifo;
    for (const auto& presentMode : physicalDevice.getSurfacePresentModesKHR(surface))
    {
        if (presentMode == vk::PresentModeKHR::eMailbox)
        {
            presentationMode = presentMode;
            break;
        }

        if (presentMode == vk::PresentModeKHR::eImmediate)
        {
            presentationMode = presentMode;
        }
    }


    // Create SwapChain
    vk::SwapchainCreateInfoKHR swapChainCreateInfo({},
                                                   surface,
                                                   surfaceCapabilities.minImageCount,
                                                   _colorFormat,
                                                   surfaceFormat.colorSpace,
                                                   swapchainExtent,
                                                   1,
                                                   usage,
                                                   vk::SharingMode::eExclusive,
                                                   {},
                                                   preTransform,
                                                   compositeAlpha,
                                                   presentationMode,
                                                   true,
                                                   oldSwapChain);

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        uint32_t queueFamilyIndices[2] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
        // If the graphics and present queues are from different queue families, we either have to explicitly transfer
        // ownership of images between the queues, or we have to create the swapchain with imageSharingMode as
        // vk::SharingMode::eConcurrent
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    _swapChain = device.createSwapchainKHR(swapChainCreateInfo);

    _images = device.getSwapchainImagesKHR(_swapChain);

    _imageViews.reserve(_images.size());
    vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, _colorFormat, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
    for (auto image : _images)
    {
        imageViewCreateInfo.image = image;
        _imageViews.push_back(device.createImageView(imageViewCreateInfo));
    }
}

void VulkanDevice::SwapChainData::clear(vk::Device const& device)
{
    for (auto& imageView : _imageViews)
    {
        device.destroyImageView(imageView);
    }
    _imageViews.clear();
    _images.clear();
    device.destroySwapchainKHR(_swapChain);
}
