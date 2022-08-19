#pragma once
#include <iostream>

#include <vulkan/vulkan.h>
#include "vkDevice.h"
#include "vkTexture.h"
#include "vkDeviceBuffer.h"
#include "BasicIO.h"

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4100)
    #pragma warning(disable: 4505)
    #pragma warning(disable: 4996)
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC 1
#include <MaterialXRender/External/StbImage/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC 1
#include <MaterialXRender/External/StbImage/stb_image_write.h>

#include <algorithm>
#include <cstring>

static VkShaderModule CreateShaderModule(VkDevice device, std::string shader_file)
{
  VkShaderModule module;
  // Create shader module with compiled SPIRV shader
  uint32_t filelength;
  auto code = ReadBinaryFile(filelength, shader_file.c_str());
  if( code == nullptr )
      throw std::runtime_error("Error reading binary shader file.");
      
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pCode = code;
  createInfo.codeSize = filelength;
  
  VK_ERROR_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &module));

  return module;
}

static VkShaderModule CreateShaderModule(VkDevice device, const uint32_t *code, uint32_t codeSize)
{
    VkShaderModule module;
    // Create shader module with compiled SPIRV shader
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode = code;
    createInfo.codeSize = codeSize;

    VK_ERROR_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &module));

    return module;
}

static VkCommandBuffer CreateSingleCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void SubmitFreeCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence;
    VK_ERROR_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence));

    VK_ERROR_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_ERROR_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void CopyBufferToImage(VulkanDevicePtr device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBuffer commandBuffer = CreateSingleCommandBuffer(device->GetDevice(), device->GetCommandPool());

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, depth };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    SubmitFreeCommandBuffer(device->GetDevice(), device->GetCommandPool(), device->GetDefaultQueue(), commandBuffer);
}

static void CopyImageToBuffer(VulkanDevicePtr device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBuffer commandBuffer = CreateSingleCommandBuffer(device->GetDevice(), device->GetCommandPool());

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, depth };

    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    SubmitFreeCommandBuffer(device->GetDevice(), device->GetCommandPool(), device->GetDefaultQueue(), commandBuffer);
}

static VkImageView CreateImageView(VulkanDevicePtr device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType imageType)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = imageType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_ERROR_CHECK(vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView));

    return imageView;
}

static void WriteDescriptorImage(VulkanDevicePtr device, VkImageView view, VkSampler sampler, VkDescriptorType descriptorType, uint32_t binding, VkDescriptorSet& descriptorSet, VkImageLayout layout= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
{
    // Specify the buffer to bind to the descriptor.
    VkDescriptorImageInfo descriptorImageInfo = {};
    descriptorImageInfo.imageLayout = layout;
    descriptorImageInfo.imageView = view;
    descriptorImageInfo.sampler = sampler;

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = descriptorType;
    writeDescriptorSet.pImageInfo = &descriptorImageInfo;

    // perform the update of the descriptor set.
    vkUpdateDescriptorSets(device->GetDevice(), 1, &writeDescriptorSet, 0, NULL);
}

static VkSampler CreateDefaultSampler(VulkanDevicePtr device, bool normalizeCoordinates)
{
    VkSampler sampler;
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.unnormalizedCoordinates = normalizeCoordinates ? VK_FALSE : VK_TRUE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 0.f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VK_ERROR_CHECK(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler));

    return sampler;
}

static void WriteDescriptorBuffer(VulkanDevicePtr device, VkBuffer buffer, VkDeviceSize bufferSize, VkDescriptorType descriptorType, uint32_t binding, VkDescriptorSet descriptorSet, uint32_t offset)
{
    // Specify the buffer to bind to the descriptor.
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = buffer;
    descriptorBufferInfo.offset = offset;
    descriptorBufferInfo.range = bufferSize;

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = descriptorType;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    // perform the update of the descriptor set.
    vkUpdateDescriptorSets(device->GetDevice(), 1, &writeDescriptorSet, 0, NULL);
}

static std::shared_ptr<VulkanTexture> LoadTextureFromImageFile(VulkanDevicePtr device, std::string fileName, bool hdr)
{
    //TODO: Add ImageHandler
#if 1
    VkFormat textureFormat = VK_FORMAT_UNDEFINED;
    int channels;
    glm::ivec3 imageDimensions(1);
    stbi_set_flip_vertically_on_load(false);
    unsigned char* pixelData = nullptr;

    // load floating point data if this is HDR
    if (hdr)
        pixelData = reinterpret_cast<unsigned char*>(stbi_loadf(fileName.c_str(), &imageDimensions.x, &imageDimensions.y, &channels, 0));
    else
        pixelData = stbi_load(fileName.c_str(), &imageDimensions.x, &imageDimensions.y, &channels, 0);

    if (pixelData == nullptr)
        throw std::runtime_error("Failed to load " + fileName);

    auto componentSize = hdr ? sizeof(float) : sizeof(unsigned char);

    //  use floating point formats for HDR textures
    auto getTextureFormat = [&hdr](size_t channels) -> VkFormat
    {
        switch (channels)
        {
        case 1:
            return hdr ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8_UNORM;
        case 2:
            return hdr ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R8G8_UNORM;
        case 3:
            return hdr ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R8G8B8_UNORM;
        case 4:
            return hdr ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
        default:
            throw std::runtime_error("Unsupported number of texture channels " + std::to_string(channels));
        }
    };

    // try and find an appropriate format, if one can't be found then try padding it out
    // i.e. if there is no supported 3 component format then try a 4 component one, if that exists
    // then pad the data from 3 components to 4
    size_t padding = 0;
    while (channels + padding <= 4)
    {
        auto testFormat = getTextureFormat(static_cast<size_t>(channels) + padding);
        //textureFormat = device->FindSupportedFormat({ testFormat }, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        //if (textureFormat != VK_FORMAT_UNDEFINED)
        //    break;
        textureFormat = device->FindSupportedFormat({ testFormat }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        if (textureFormat != VK_FORMAT_UNDEFINED)
            break;
        padding += 1;
    }

    if (textureFormat == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Unable to find suitable image format.");

    // if we need to pad the data then do so
    unsigned char* paddedPixelData;
    if (padding > 0)
    {
        size_t ch = static_cast<size_t>(channels);
        size_t x = static_cast<size_t>(imageDimensions.x);
        size_t y = static_cast<size_t>(imageDimensions.y);
        // new set of pixel data
        if (hdr)
            paddedPixelData = reinterpret_cast<unsigned char*>(new float[x * y * (ch + padding)]);
        else
            paddedPixelData = new unsigned char[x * y * (ch + padding)];

        // copy each element from the unpadded data to the padded data
        for (size_t i = 0; i < static_cast<size_t>(x * y); ++i)
        {
            auto pixelDataOffset = i * ch * componentSize;
            auto paddedPixelDataOffset = i * (ch + padding) * componentSize;
            memcpy(paddedPixelData + paddedPixelDataOffset, reinterpret_cast<unsigned char*>(pixelData) + pixelDataOffset, ch* componentSize);
        }

        // free the original data and set the pointer to the padded data
        stbi_image_free(pixelData);
        pixelData = paddedPixelData;
    }

    auto pixelDataBuffer = device->CreateDeviceBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_FORMAT_UNDEFINED);
    pixelDataBuffer->Write(static_cast<void*>(pixelData), imageDimensions.x* imageDimensions.y* (channels + padding)* componentSize);
    auto texture = std::make_shared<VulkanTexture>(device, textureFormat, imageDimensions, true);
    texture->Initialize(VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    texture->CopyBufferToTexture(pixelDataBuffer);
    if (padding == 0)
        stbi_image_free(pixelData);

    return texture;
#endif

}

template<class T>
inline T alignUp(T size, size_t alignment) noexcept
{
    if (alignment > 0)
    {
        assert(((alignment - 1) & alignment) == 0);
        T mask = static_cast<T>(alignment - 1);
        return (size + mask) & ~mask;
    }
    return size;
}
