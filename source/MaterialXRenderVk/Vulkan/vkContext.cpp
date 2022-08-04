#if 0
//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <windows.h>
#include <MaterialXRenderVk/Export.h>
#include <MaterialXRenderVk/VkContext.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type,
                                                     uint64_t object, size_t location, int32_t message_code,
                                                     const char* layer_prefix, const char* message, void* user_data)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        printf("Validation Layer: Error: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        printf("Validation Layer: Warning: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        printf("Validation Layer: Performance warning: %s: %s", layer_prefix, message);
    }
    else
    {
        printf("Validation Layer: Information: %s: %s", layer_prefix, message);
    }
    return VK_FALSE;
}

MATERIALX_NAMESPACE_BEGIN

VkContext::VkContext(SimpleWindowPtr window, HardwareContextHandle /*sharedWithContext*/):
    _window(window),
    _contextHandle(nullptr),
    _isValid(false)
{
    // Get the existing window wrapper.
    WindowWrapperPtr windowWrapper = _window->getWindowWrapper();
    if (!windowWrapper->isValid())
    {
        return;
    }

    
    createInstance();
    createDevice();
    //createSwapChain();
    //_swapChain = VkSwapChain(_instance, _physicalDevice, _device);
}

VkContext::~VkContext()
{
}

int VkContext::makeCurrent()
{
    return 0;
}


// Vulkan Methods:
void VkContext::createInstance()
{
    // Vulkan device
    vk::Device device;
    vk::PhysicalDevice physicalDevice;

    vk::ApplicationInfo appInfo("MaterialX Vulkan Render", {}, "MTLX_RENDER", VK_MAKE_VERSION(1, 0, 0));
    
    // Layers and extension
    std::vector<const char*> validationLayers;
    std::vector<vk::LayerProperties> supported_validation_layers = vk::enumerateInstanceLayerProperties();
    for (vk::LayerProperties prop : supported_validation_layers)
    {
        if (strcmp("VK_LAYER_KHRONOS_validation", prop.layerName) == 0)
        {
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
            break;
        }
    }
    if (validationLayers.empty())
        throw std::runtime_error("Layer VK_LAYER_KHRONOS_validation not supported\n");


    //= { VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
    std::vector<const char*> instanceExtensions;
    std::vector<vk::ExtensionProperties> supported_extensions = vk::enumerateInstanceExtensionProperties();
    for (vk::ExtensionProperties prop : supported_extensions)
    {
        if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            break;
        }
    }
    if (instanceExtensions.empty())
        throw std::runtime_error("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n");

    vk::InstanceCreateInfo instance_info({}, &appInfo, validationLayers, instanceExtensions);
    // Debug only
    /*
    vk::DebugReportCallbackCreateInfoEXT debug_report_create_info(, debug_callback);
    instance_info.pNext = &debug_report_create_info;
    */
    // Debug only

    _instance = vk::createInstance(instance_info);
    // initialize function pointers for instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

    //_debug_callback = _instance.createDebugReportCallbackEXT(debug_report_create_info);
}

void VkContext::createDevice()
{
    std::vector<vk::PhysicalDevice> physicalDevices = _instance.enumeratePhysicalDevices();
    assert(physicalDevices.size() > 0);
    // use first physical device
    _physicalDevice = physicalDevices[0];
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = _physicalDevice.getQueueFamilyProperties();

    // find a graphics queue
    uint32_t graphicsQueueFamilyIndex = 0;
    for (size_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            graphicsQueueFamilyIndex = (uint32_t) i;
            break;
        }
    }

    const float defaultQueuePriority(0.0f);
    vk::DeviceQueueCreateInfo queueCreatInfo({}, graphicsQueueFamilyIndex, 1, &defaultQueuePriority);

    // Create the logical device representation
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreatInfo;
    deviceCreateInfo.pEnabledFeatures = nullptr;

    if (deviceExtensions.size() > 0)
    {
        deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }

    _device = _physicalDevice.createDevice(deviceCreateInfo);

    // Get a graphics queue from the device
    _queue = _device.getQueue(graphicsQueueFamilyIndex, 0);
}

void VkContext::createSwapChain()
{
    //_swapChain = new SwapChain(instance, physicalDevice, device);
    //_swapChain->createSurface(windowInstance, window);
    // swapChain->create(&windowSize.width, &windowSize.height);
}

MATERIALX_NAMESPACE_END
#endif