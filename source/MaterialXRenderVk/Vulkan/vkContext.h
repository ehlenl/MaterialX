//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_VKCONTEXT_H
#define MATERIALX_VKCONTEXT_H

/// @file
#include <MaterialXRenderHw/SimpleWindow.h>

//#include <MaterialXRenderVk/Export.h>
//#include <MaterialXRenderVk/VkSwapChain.h>
#include <MaterialXRenderVk/Vulkan/vkDevice.h>



#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_win32.h>

MATERIALX_NAMESPACE_BEGIN

using HardwareContextHandle = void*;

/// SimpleWindow shared pointer
using SimpleWindowPtr = std::shared_ptr<class SimpleWindow>;

/// VkContext shared pointer
using VkContextPtr = std::shared_ptr<class VkContext>;


/// @class VkContext
/// An Vulkan context singleton
class MX_RENDERVK_API VkContext
{
  public:
    /// Create a new context
    static VkContextPtr create(SimpleWindowPtr window, HardwareContextHandle context = {})
    {
        return VkContextPtr(new VkContext(window, context));
    }

    /// Default destructor
    virtual ~VkContext();

    /// Return OpenGL context handle
    HardwareContextHandle contextHandle() const
    {
        return _contextHandle;
    }

    /// Return if context is valid
    bool isValid() const
    {
        return _isValid;
    }

    /// Make the context "current" before execution of OpenGL operations
    int makeCurrent();

    void initializeVulkan(bool validate);


  protected:
    // Create the base context. A OpenGL context to share with can be passed in.
    VkContext(SimpleWindowPtr window, HardwareContextHandle context = 0);

    // Create Vulkan Instance
    void createInstance();

    // Create Vulkan Device
    void createDevice();

    // Create Vulkan Swapchain
    void createSwapChain();


    // Simple window
    SimpleWindowPtr _window;

    // Context handle
    HardwareContextHandle _contextHandle;

    // Flag to indicate validity
    bool _isValid;


    std::vector<const char*> enabled_layers;
    std::vector<char const*> extension_names;

#if defined(__linux__) || defined(__FreeBSD__)
    // An X window used by context operations
    Window _xWindow;

    // An X display used by context operations
    Display* _xDisplay;
#endif

 public:
    /// Vulkan Objects
    /// The Vulkan instance.
    vk::Instance _instance;

    vk::SurfaceKHR _surface;

    //vk::PhysicalDevice _physicalDevice;

    /// The Vulkan device.
    VulkanDevicePtr _device;

    /// The Vulkan device queue.
    //vk::Queue _queue;

    //vk::DebugReportCallbackEXT debug_callback;
    //VkSwapChain _swapChain;
};

MATERIALX_NAMESPACE_END

#endif
