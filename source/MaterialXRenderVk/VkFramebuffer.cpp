//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXRenderVk/VkFramebuffer.h>

MATERIALX_NAMESPACE_BEGIN

//
// VkFrameBuffer methods
//

VkFrameBufferPtr VkFrameBuffer::create(
    unsigned int width, unsigned int height,
    unsigned channelCount,
    Image::BaseType baseType)
{
    return VkFrameBufferPtr(new VkFrameBuffer(width, height, channelCount, baseType));
}

VkFrameBuffer::VkFrameBuffer(
    unsigned int width, unsigned int height,
    unsigned int channelCount,
    Image::BaseType baseType) :
    _width(0),
    _height(0),
    _channelCount(channelCount),
    _baseType(baseType),
    //_encodeSrgb(encodeSrgb),
    //_device(device),
    //_colorTexture(colorTexture),
    _depthTexture(0)
{
    StringVec errors;
    const string errorType("Vulkan target creation failure.");

    // resize(width, height, true, pixelFormat, colorTexture);
}

VkFrameBuffer::~VkFrameBuffer()
{
    //[_colorTexture release];
    //[_depthTexture release];
}

void VkFrameBuffer::resize(unsigned int width, unsigned int height, bool forceRecreate)
{
    if (width * height <= 0)
    {
        return;
    }
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
