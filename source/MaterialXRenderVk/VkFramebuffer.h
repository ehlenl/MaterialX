//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_VKFRAMEBUFFER_H
#define MATERIALX_VKFRAMEBUFFER_H

/// @file
/// Vulkan framebuffer handling

#include <MaterialXRenderVk/Export.h>

#include <MaterialXRender/ImageHandler.h>

#ifdef _USE_VULKAN_CPP
#include <MaterialXRenderVk/Vulkan/vkDevice.h>
#endif

MATERIALX_NAMESPACE_BEGIN

/// Shared pointer to a VkFrameBuffer
using VkFrameBufferPtr = std::shared_ptr<class VkFrameBuffer>;

/// @class VkFrameBuffer
/// Wrapper for an Vulkan framebuffer
class MX_RENDERVK_API VkFrameBuffer
{
  public:
    /// Create a new framebuffer
    static VkFrameBufferPtr create(VulkanDevicePtr vulkanDevice,
                                   unsigned int width, 
                                   unsigned int height, 
                                   unsigned int channelCount, 
                                   Image::BaseType baseType);

    /// Destructor
    virtual ~VkFrameBuffer();

    /// Resize the framebuffer
    void resize(unsigned int width, unsigned int height);
    
    /// Return the framebuffer width
    unsigned int getWidth() const { return _width; }

    /// Return the framebuffer height
    unsigned int getHeight() const { return _height; }

    /// Bind the framebuffer for rendering.
    // void bind(MTLRenderPassDescriptor* renderpassDesc);
    void bind();

    /// Unbind the frame buffer after rendering.
    void unbind();

    /// Return our color texture handle.
    unsigned int getColorTexture() const
    {
        return _colorTexture;
    }

    /*
        void setColorTexture(id<MTLTexture> newColorTexture)
        {
            auto sameDim = [](id<MTLTexture> tex0, id<MTLTexture> tex1) -> bool
            {
                return  [tex0 width]  == [tex1 width] &&
                        [tex0 height] == [tex1 height];
            };
            if((!_colorTextureOwned || sameDim(_colorTexture, newColorTexture)) &&
               sameDim(newColorTexture, _depthTexture))
            {
                if(_colorTextureOwned)
                    [_colorTexture release];
                _colorTexture = newColorTexture;
            }
        }
    */
    /// Return our depth texture handle.
    unsigned int getDepthTexture() const
    {
        return _depthTexture;
    }

    /// Return the color data of this framebuffer as an image.
    /// If an input image is provided, it will be used to store the color data;
    /// otherwise a new image of the required format will be created.
    // ImagePtr getColorImage(id<MTLCommandQueue> cmdQueue = nil, ImagePtr image = nullptr);
    ImagePtr getColorImage(ImagePtr image = nullptr);

  protected:
    VkFrameBuffer(VulkanDevicePtr vulkanDevice, unsigned int width, unsigned int height, 
        unsigned int channelCount, Image::BaseType baseType);

  protected:
    unsigned int _width;
    unsigned int _height;
    unsigned int _channelCount;
    Image::BaseType _baseType;
    bool _encodeSrgb;

    unsigned int _framebuffer;
    unsigned int _colorTexture;
    unsigned int _depthTexture;
    VulkanDevicePtr _vulkanDevice = nullptr;

    // FrameBufferAttachment needs VkImage, VkDeviceMemory, VkImageView
    struct FrameBufferAttachment
    {
        vk::Image _image = nullptr;
        vk::DeviceMemory _deviceMemory = nullptr;
        vk::ImageView _imageView = nullptr;
    };

    FrameBufferAttachment _colorBufferAttachment;
    FrameBufferAttachment _depthBufferAttachment;

    bool _colorTextureOwned = false;
};

MATERIALX_NAMESPACE_END
#endif
