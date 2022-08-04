//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_VKFRAMEBUFFER_H
#define MATERIALX_VKFRAMEBUFFER_H

/// @file
/// OpenGL framebuffer handling

#include <MaterialXRenderVk/Export.h>
#include <MaterialXRenderVk/VkContext.h>
#include <MaterialXRender/ImageHandler.h>

#include <vulkan/vulkan.hpp>

MATERIALX_NAMESPACE_BEGIN

class VkFramebuffer;

/// Shared pointer to a VkFramebuffer
using VkFramebufferPtr = std::shared_ptr<VkFramebuffer>;

/// @class VkFramebuffer
/// Wrapper for an OpenGL framebuffer
class MX_RENDERVK_API VkFramebuffer
{
  public:
    /// Create a new framebuffer
    static VkFramebufferPtr create(unsigned int width, unsigned int height, unsigned int channelCount, Image::BaseType baseType);

    /// Destructor
    virtual ~VkFramebuffer();

    /// Resize the framebuffer
    void resize(unsigned int width, unsigned int height);

    /// Set the encode sRGB flag, which controls whether values written
    /// to the framebuffer are encoded to the sRGB color space.
    void setEncodeSrgb(bool encode)
    {
        _encodeSrgb = encode;
    }

    /// Return the encode sRGB flag.
    bool getEncodeSrgb()
    {
        return _encodeSrgb;
    }

    /// Bind the framebuffer for rendering.
    void bind();

    /// Unbind the frame buffer after rendering.
    void unbind();

    /// Return our color texture handle.
    unsigned int getColorTexture() const
    {
        return _colorTexture;
    }

    /// Return our depth texture handle.
    unsigned int getDepthTexture() const
    {
        return _depthTexture;
    }

    /// Return the color data of this framebuffer as an image.
    /// If an input image is provided, it will be used to store the color data;
    /// otherwise a new image of the required format will be created.
    ImagePtr getColorImage(ImagePtr image = nullptr);

    /// Blit our color texture to the back buffer.
    void blit();


    void createFrameBufferAttachments(VkContextPtr ctx);

  protected:
    VkFramebuffer(unsigned int width, unsigned int height, unsigned int channelCount, Image::BaseType baseType);

  protected:
    unsigned int _width;
    unsigned int _height;
    unsigned int _channelCount;
    Image::BaseType _baseType;
    bool _encodeSrgb;

    unsigned int _framebuffer;
    unsigned int _colorTexture;
    unsigned int _depthTexture;

    // Vulkan Objects
  public:

    // Framebuffer for offscreen rendering
    struct FrameBufferAttachment
    {
        vk::Image image;
        vk::DeviceMemory mem;
        vk::ImageView view;
        vk::Format format;

        FrameBufferAttachment() = default;

        FrameBufferAttachment(vk::Image image_, vk::DeviceMemory mem_, vk::ImageView view_, vk::Format format_) :
            image(image_), mem(mem_), view(view_), format(format_)
        {
        }

        void destroy(vk::Device device)
        {
            device.destroyImageView(view);
            device.destroyImage(image);
            device.freeMemory(mem);
        }
    };

    vk::Framebuffer _vkframebuffer;
    FrameBufferAttachment colorAttachment, depthAttachment;
};

MATERIALX_NAMESPACE_END

#endif
