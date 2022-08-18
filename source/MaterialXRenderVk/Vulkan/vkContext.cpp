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

void VkContext::initializeVulkan(bool validate)
{
    // Create Vulkan Instance
    _device = VulkanDevice::create();

    // Create Surface
    HINSTANCE hInstance = GetModuleHandle(NULL);
    auto const createInfo = vk::Win32SurfaceCreateInfoKHR()
                                .setHinstance(hInstance)
                                .setHwnd(_window->getWindowWrapper()->externalHandle());
    _surface = vk::SurfaceKHR();
    _instance = _device->GetInstance();
    vk::Result sresult = _instance.createWin32SurfaceKHR(&createInfo, nullptr, &_surface);
    assert(sresult == vk::Result::eSuccess);
    
    // Create and initialize Device
    _device->InitializeDevice(_surface);

}

MATERIALX_NAMESPACE_END
