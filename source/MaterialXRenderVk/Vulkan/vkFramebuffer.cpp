#if 0
//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXRenderVk/VkFramebuffer.h>

#include <MaterialXRenderVk/VkContext.h>
#include <MaterialXRenderVk/VkProgram.h>
#include <MaterialXRenderVk/VkRenderer.h>
//#include <MaterialXRenderVk/VkTextureHandler.h>

//#include <MaterialXRenderVk/External/Glad/glad.h>

MATERIALX_NAMESPACE_BEGIN

//
// VkFramebuffer methods
//

VkFramebufferPtr VkFramebuffer::create(unsigned int width, unsigned int height, unsigned channelCount, Image::BaseType baseType)
{
    return VkFramebufferPtr(new VkFramebuffer(width, height, channelCount, baseType));
}

VkFramebuffer::VkFramebuffer(unsigned int width, unsigned int height, unsigned int channelCount, Image::BaseType baseType) :
    _width(width),
    _height(height),
    _channelCount(channelCount),
    _baseType(baseType),
    _encodeSrgb(false),
    _framebuffer(0),
    _colorTexture(0),
    _depthTexture(0)
{
    if (!glGenFramebuffers)
    {
        gladLoadGL();
    }

    // Convert texture format to OpenGL.
    int glType, glFormat, glInternalFormat;
    VkTextureHandler::mapTextureFormatToGL(baseType, channelCount, true, glType, glFormat, glInternalFormat);

    // Create and bind framebuffer.
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

    // Create the offscreen color target and attach to the framebuffer.
    glGenTextures(1, &_colorTexture);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, _width, _height, 0, glFormat, glType, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture, 0);

    // Create the offscreen depth target and attach to the framebuffer.
    glGenTextures(1, &_depthTexture);
    glBindTexture(GL_TEXTURE_2D, _depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

    glBindTexture(GL_TEXTURE_2D, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
    glDrawBuffer(GL_NONE);

    // Validate the framebuffer.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = VkProgram::UNDEFINED_OPENGL_RESOURCE_ID;

        string errorMessage;
        switch (status)
        {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            errorMessage = "GL_FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            errorMessage = "GL_FRAMEBUFFER_UNDEFINED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            errorMessage = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            break;
        default:
            errorMessage = std::to_string(status);
            break;
        }

        throw ExceptionRenderError("Frame buffer object setup failed: " + errorMessage);
    }

    // Unbind on cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);

}



void VkFramebuffer::createFrameBufferAttachments(VkContextPtr ctx)
{
    // Color attachment
    vk::Format colorbufferFormat = vk::Format::eR32G32B32A32Sfloat;

    vk::ImageCreateInfo image_create_info;
    image_create_info.imageType = vk::ImageType::e2D;
    image_create_info.format = colorbufferFormat;
    image_create_info.extent = vk::Extent3D({ _width, _height }, 1);
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = vk::SampleCountFlagBits::e1;
    image_create_info.tiling = vk::ImageTiling::eOptimal;
    image_create_info.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    vk::Image image = ctx->_device.createImage(image_create_info);

    vk::MemoryRequirements memory_requirements = ctx->_device.getImageMemoryRequirements(image);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(ctx->_physicalDevice, &memory_properties);
    /*
  
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((gpu.get_memory_properties().memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memory_type_found)
				{
					*memory_type_found = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memory_type_found)
	{
		*memory_type_found = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
    */


    //TODO: FIX ME -ASHWIN
    /*
    vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
                                                ctx->_device.get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    
    vk::DeviceMemory mem = ctx->_device.allocateMemory(memory_allocate_info);

    ctx->_device.bindImageMemory(image, mem, 0);

    vk::ImageViewCreateInfo image_view_create_info({}, image, vk::ImageViewType::e2D, colorbufferFormat, {}, { vk::ImageLayout::eColorAttachmentOptimal, 0, 1, 0, 1 });
    vk::ImageView view = ctx->_device.createImageView(image_view_create_info);
    */

}

VkFramebuffer::~VkFramebuffer()
{
    if (_framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
        glDeleteTextures(1, &_colorTexture);
        glDeleteTextures(1, &_depthTexture);
        glDeleteFramebuffers(1, &_framebuffer);
    }
}

void VkFramebuffer::resize(unsigned int width, unsigned int height)
{
    if (width * height == 0)
    {
        return;
    }
    if (width != _width || _height != height)
    {
        unbind();

        int glType, glFormat, glInternalFormat;
        VkTextureHandler::mapTextureFormatToGL(_baseType, _channelCount, true, glType, glFormat, glInternalFormat);

        glBindTexture(GL_TEXTURE_2D, _colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, glFormat, glType, nullptr);

        glBindTexture(GL_TEXTURE_2D, _depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_2D, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
        glDrawBuffer(GL_NONE);

        _width = width;
        _height = height;
    }
}

void VkFramebuffer::bind()
{
    if (!_framebuffer)
    {
        throw ExceptionRenderError("No framebuffer exists to bind");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    GLenum colorList[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, colorList);

    if (_encodeSrgb)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    else
    {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    glViewport(0, 0, _width, _height);
}

void VkFramebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
    glDrawBuffer(GL_NONE);
}

ImagePtr VkFramebuffer::getColorImage(ImagePtr image)
{
    if (!image)
    {
        image = Image::create(_width, _height, _channelCount, _baseType);
        image->createResourceBuffer();
    }

    int glType, glFormat, glInternalFormat;
    VkTextureHandler::mapTextureFormatToGL(_baseType, _channelCount, false, glType, glFormat, glInternalFormat);

    bind();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, image->getWidth(), image->getHeight(), glFormat, glType, image->getResourceBuffer());
    unbind();

    return image;
}

void VkFramebuffer::blit()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

MATERIALX_NAMESPACE_END

#endif