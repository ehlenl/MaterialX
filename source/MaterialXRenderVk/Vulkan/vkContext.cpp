#include <windows.h>
#include <assert.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include <MaterialXRenderVk/Export.h>
#include <MaterialXRenderVk/Vulkan/VkContext.h>

MATERIALX_NAMESPACE_BEGIN

VkContext::VkContext(SimpleWindowPtr window, HardwareContextHandle /*sharedWithContext*/) :
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

    // createInstance();
    // createDevice();
    // createSwapChain();
    //_swapChain = VkSwapChain(_instance, _physicalDevice, _device);
}

VkContext::~VkContext()
{
}

void VkContext::init_vk(bool validate)
{
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    char const* const instance_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
    enabled_extension_count = 0;
    enabled_layer_count = 0;

    // Look for validation layers
    vk::Bool32 validation_found = VK_FALSE;
    if (validate)
    {
        auto result = vk::enumerateInstanceLayerProperties(&instance_layer_count, static_cast<vk::LayerProperties*>(nullptr));
        assert(result == vk::Result::eSuccess);

        if (instance_layer_count > 0)
        {
            std::vector<vk::LayerProperties> instance_layers(instance_layer_count);
            result = vk::enumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data());
            assert(result == vk::Result::eSuccess);

            for (vk::LayerProperties prop : instance_layers)
            {
                std::cout << prop.layerName << std::endl;
                if (strcmp("VK_LAYER_KHRONOS_validation", prop.layerName) == 0)
                {
                    validation_found = true;
                    break;
                }
            }
            if (validation_found)
            {
                enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
            }
        }

        if (!validation_found)
        {
            throw std::runtime_error("vkEnumerateInstanceLayerProperties failed to find required validation layer.");
        }
    }

    /* Look for instance extensions */
    vk::Bool32 surfaceExtFound = VK_FALSE;
    vk::Bool32 platformSurfaceExtFound = VK_FALSE;
    extension_names.clear();

    auto result = vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count,
                                                           static_cast<vk::ExtensionProperties*>(nullptr));
    assert(result == vk::Result::eSuccess);

    if (instance_extension_count > 0)
    {
        std::vector<vk::ExtensionProperties> instance_extensions(instance_extension_count);
        result = vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data());
        assert(result == vk::Result::eSuccess);


        for (vk::ExtensionProperties prop : instance_extensions)
        {
            if (!strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, prop.extensionName))
            {
                extension_names.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, prop.extensionName))
            {
                surfaceExtFound = 1;
                extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, prop.extensionName))
            {
                platformSurfaceExtFound = 1;
                extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            }
#endif
        }
    }

    if (!surfaceExtFound)
    {
        throw std::runtime_error("vkEnumerateInstanceExtensionProperties failed to find the VK_KHR_SURFACE extension");
    }

    if (!platformSurfaceExtFound)
    {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        throw std::runtime_error("vkEnumerateInstanceExtensionProperties failed to find the VK_KHR_WIN32_SURFACE_EXTENSION_NAME"); 
#endif
    }
    auto const app = vk::ApplicationInfo()
                         .setPApplicationName("MaterialXRender")
                         .setApplicationVersion(0)
                         .setPEngineName("MaterialXRender")
                         .setEngineVersion(0)
                         .setApiVersion(VK_API_VERSION_1_2);
    auto const inst_info = vk::InstanceCreateInfo()
                               .setPApplicationInfo(&app)
                               .setEnabledLayerCount(enabled_layer_count)
                               .setPpEnabledLayerNames(instance_validation_layers)
                               .setEnabledExtensionCount(enabled_extension_count)
                               .setPpEnabledExtensionNames(extension_names.data());

    result = vk::createInstance(&inst_info, nullptr, &_instance);
    if (result == vk::Result::eErrorIncompatibleDriver)
    {
        throw std::runtime_error("Cannot find a compatible Vulkan installable client driver (ICD)");
    }
    else if (result == vk::Result::eErrorExtensionNotPresent)
    {
        throw std::runtime_error("Cannot find a specified extension library");
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("vkCreateInstance failed");
    }

    // CHECKPOINT #1
    // Craete Device
    /*
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    uint32_t deviceCount;
    VK_ERROR_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, NULL));
    if (deviceCount == 0)
        throw std::runtime_error("Could not find a device with vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::vector<VkPhysicalDevice> supportedDevices;

    for (VkPhysicalDevice device : devices)
    {
        SwapChainSupportDetails swapChainSupport;
        if (this->QuerySwapChainSupport(swapChainSupport, device))
        {
            if (!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty())
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
    for (size_t i = 0; i < supportedDevices.size(); ++i)
    {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(supportedDevices[i], &deviceProps);
        std::cout << i + 1 << ". " << deviceProps.deviceName << std::endl;
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
    */
}

MATERIALX_NAMESPACE_END
