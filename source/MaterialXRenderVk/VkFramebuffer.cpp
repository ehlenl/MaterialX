//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXRenderVk/VkFramebuffer.h>

MATERIALX_NAMESPACE_BEGIN

//
// VkFrameBuffer methods
//

VkFrameBufferPtr VkFrameBuffer::create(VulkanDevicePtr vulkanDevice,
    unsigned int width, unsigned int height,
    unsigned channelCount,
    Image::BaseType baseType)
{
    return VkFrameBufferPtr(new VkFrameBuffer(vulkanDevice, width, height, channelCount, baseType));
}

VkFrameBuffer::VkFrameBuffer(VulkanDevicePtr vulkanDevice,
    unsigned int width, unsigned int height,
    unsigned int channelCount,
    Image::BaseType baseType) :
    _width(0),
    _height(0),
    _channelCount(channelCount),
    _baseType(baseType),
    //_encodeSrgb(encodeSrgb),
    _vulkanDevice(vulkanDevice),
    //_colorTexture(colorTexture),
    _depthTexture(0)
{
    StringVec errors;
    const string errorType("Vulkan target creation failure.");

    resize(width, height);
}

VkFrameBuffer::~VkFrameBuffer()
{
    //[_colorTexture release];
    //[_depthTexture release];
}

void VkFrameBuffer::resize(unsigned int width, unsigned int height)
{
    if (width * height <= 0)
    {
        return;
    }

    //NOTES:
    //  VkFramebuffer + VkRenderPass defines the render target.
    //  Renderpass defines which attachment will be written with colors.
    //  VkFramebuffer defines which VkImageView is to be which attachment.
    //  VkImageView defines which part of VkImage to use.
    //  VkImage defines which VkDeviceMemory is used and a format of the texel.
    
    vk::Device vulkanDevice = _vulkanDevice->GetDevice(); 
    // Create Vulkan Image for ColorBuffer
    const vk::Format colorFormat = vk::Format::eR8G8B8A8Unorm;
    _colorBufferAttachment._image = vulkanDevice.createImage(vk::ImageCreateInfo(vk::ImageCreateFlags(),
                                                                                 vk::ImageType::e2D,
                                                                                 colorFormat,
                                                                                 vk::Extent3D(width, height, 1),
                                                                                 1,
                                                                                 1,
                                                                                 vk::SampleCountFlagBits::e1,
                                                                                 vk::ImageTiling::eOptimal,
                                                                                 vk::ImageUsageFlagBits::eColorAttachment |
                                                                                 vk::ImageUsageFlagBits::eTransferSrc));
    // Allocate memory for ColorBuffer
    vk::MemoryRequirements colormemoryRequirements = vulkanDevice.getImageMemoryRequirements(_colorBufferAttachment._image);
    uint32_t memoryTypeIndex = _vulkanDevice->FindMemoryType(colormemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    _colorBufferAttachment._deviceMemory = vulkanDevice.allocateMemory(vk::MemoryAllocateInfo(colormemoryRequirements.size, memoryTypeIndex));
    // Bind memory to image
    vulkanDevice.bindImageMemory(_colorBufferAttachment._image, _colorBufferAttachment._deviceMemory, 0);
    // Create View
    _colorBufferAttachment._imageView = vulkanDevice.createImageView(vk::ImageViewCreateInfo(
                                                                                vk::ImageViewCreateFlags(), 
                                                                                _colorBufferAttachment._image, 
                                                                                vk::ImageViewType::e2D, 
                                                                                colorFormat,
                                                                                {},
                                                                                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }));


    // For depth format, get tiling property
    //TODO: Try other ones vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint or vk::Format::eD16Unorm
    const vk::Format depthFormat = vk::Format::eD16Unorm;
    vk::FormatProperties formatProperties = _vulkanDevice->GetPhysicalDevice().getFormatProperties(depthFormat);
    vk::ImageTiling tiling;
    if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
    {
        tiling = vk::ImageTiling::eLinear;
    }
    else if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
    {
        tiling = vk::ImageTiling::eOptimal;
    }
    else
    {
        throw std::runtime_error("DepthStencilAttachment is not supported for eD32Sfloat depth format.");
    }

    //////////////////////////////////////////////////////////////////////////
    // Create Image
    vk::ImageCreateInfo depthimageCreateInfo(vk::ImageCreateFlags(),
                                        vk::ImageType::e2D,
                                        depthFormat,
                                        vk::Extent3D(width, height, 1),
                                        1,
                                        1,
                                        vk::SampleCountFlagBits::e1,
                                        tiling,
                                        vk::ImageUsageFlagBits::eDepthStencilAttachment);
    _depthBufferAttachment._image = vulkanDevice.createImage(depthimageCreateInfo);
    
    // Allocate
    vk::MemoryRequirements depthMemoryRequirements = vulkanDevice.getImageMemoryRequirements(_depthBufferAttachment._image);
    uint32_t depthmemoryTypeIndex = _vulkanDevice->FindMemoryType(depthMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    _depthBufferAttachment._deviceMemory = vulkanDevice.allocateMemory(vk::MemoryAllocateInfo(depthMemoryRequirements.size, depthmemoryTypeIndex));
    // Bind image to memory 
    vulkanDevice.bindImageMemory(_depthBufferAttachment._image, _depthBufferAttachment._deviceMemory, 0);
    // Create View
    _depthBufferAttachment._imageView = vulkanDevice.createImageView(vk::ImageViewCreateInfo(
        vk::ImageViewCreateFlags(), _depthBufferAttachment._image, vk::ImageViewType::e2D, depthFormat, {}, { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 }));
    
    _width = width;
    _height = height;
#if 0    
    if (width != _width || _height != height || forceRecreate)
    {
        // Convert texture format to Metal
        MTLDataType    dataType;
        if(pixelFormat == MTLPixelFormatInvalid)
            MetalTextureHandler::mapTextureFormatToMetal(_baseType, _channelCount, _encodeSrgb, dataType, pixelFormat);

        MTLTextureDescriptor* texDescriptor = [MTLTextureDescriptor
                                               texture2DDescriptorWithPixelFormat:pixelFormat
                                               width:width
                                               height:height
                                               mipmapped:NO];
        [texDescriptor setStorageMode:MTLStorageModePrivate];
        [texDescriptor setUsage:MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead];
        
        if(extColorTexture == nil)
        {
            _colorTexture = [_device newTextureWithDescriptor:texDescriptor];
            _colorTextureOwned = true;
        }
        else
        {
            _colorTexture = extColorTexture;
            _colorTextureOwned = false;
        }
        
        texDescriptor.pixelFormat = MTLPixelFormatDepth32Float;
        [texDescriptor setUsage:MTLTextureUsageRenderTarget];
        _depthTexture = [_device newTextureWithDescriptor:texDescriptor];

        _width = width;
        _height = height;
    }
#endif
}

void VkFrameBuffer::bind()
{
#if 0
    [renderpassDesc.colorAttachments[0] setTexture:getColorTexture()];
    [renderpassDesc.colorAttachments[0] setLoadAction:MTLLoadActionClear];
    [renderpassDesc.colorAttachments[0] setStoreAction:MTLStoreActionStore];
    
    [renderpassDesc.depthAttachment setTexture:getDepthTexture()];
    [renderpassDesc.depthAttachment setClearDepth:1.0];
    [renderpassDesc.depthAttachment setLoadAction:MTLLoadActionClear];
    [renderpassDesc.depthAttachment setStoreAction:MTLStoreActionStore];
    [renderpassDesc setStencilAttachment:nil];
    
    [renderpassDesc setRenderTargetWidth:_width];
    [renderpassDesc setRenderTargetHeight:_height];
#endif
}

void VkFrameBuffer::unbind()
{
}

ImagePtr VkFrameBuffer::getColorImage(ImagePtr image)
{
    if (!image)
    {
        image = Image::create(_width, _height, _channelCount, _baseType);
        image->createResourceBuffer();
    }
#if 0    
    if(cmdQueue == nil)
    {
        return image;
    }
    
    size_t bytesPerRow = _width*_channelCount*MetalTextureHandler::getTextureBaseTypeSize(_baseType);
    size_t bytesPerImage = _height * bytesPerRow;
    
    id<MTLBuffer> buffer = [_device newBufferWithLength:bytesPerImage options:MTLResourceStorageModeShared];
    
    id<MTLCommandBuffer> cmdBuffer = [cmdQueue commandBuffer];
    
    id<MTLBlitCommandEncoder> blitCmdEncoder = [cmdBuffer blitCommandEncoder];
    [blitCmdEncoder copyFromTexture:_colorTexture
                        sourceSlice:0
                        sourceLevel:0
                       sourceOrigin:MTLOriginMake(0, 0, 0)
                         sourceSize:MTLSizeMake(_width, _height, 1)
                           toBuffer:buffer destinationOffset:0
             destinationBytesPerRow:bytesPerRow
           destinationBytesPerImage:bytesPerImage
                            options:MTLBlitOptionNone];
    
    
    [blitCmdEncoder endEncoding];
    
    [cmdBuffer commit];
    [cmdBuffer waitUntilCompleted];

    std::vector<unsigned char> imageData(bytesPerImage);
    memcpy(imageData.data(), [buffer contents], bytesPerImage);
    
    if([_colorTexture pixelFormat] == MTLPixelFormatBGRA8Unorm)
    {
        for(unsigned int j = 0; j < _height; ++j)
        {
            unsigned int rawStart = j * (_width * _channelCount);
            for(unsigned int i = 0; i < _width; ++i)
            {
                unsigned int offset = rawStart + _channelCount * i;
                unsigned char tmp = imageData[offset + 0];
                imageData[offset + 0] = imageData[offset + 2];
                imageData[offset + 2] = tmp;
            }
        }
    }
    
    memcpy(image->getResourceBuffer(), imageData.data(), bytesPerImage);
    [buffer release];
#endif
    return image;
}

MATERIALX_NAMESPACE_END
