#include "VkTexture.h"
#include "VkHelpers.h"
#include "VkBuffer.h"
#include "VkHostBuffer.h"

VulkanTexture::VulkanTexture(std::shared_ptr<VulkanDevice> _device, VkFormat textureFormat, const glm::uvec3 &textureDimensions, bool _generateMipmaps, bool _multisampled)
: device(_device),
    image(nullptr),
    imageView(nullptr),
    imageMemory(nullptr),
    resolvedImage(nullptr),
    resolvedImageMemory(nullptr),
    destroyImage(true),
    generateMipmaps(_generateMipmaps)
{
    device = _device;
    dimensions = textureDimensions;
    imageFormat = textureFormat;

    mipLevels = _generateMipmaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(dimensions.x, dimensions.y)))) + 1 : 1;

    sampleCount = _multisampled ? this->device->GetMaxMultisampleCount() : VK_SAMPLE_COUNT_1_BIT;
}

VulkanTexture::~VulkanTexture()
{
    if( imageView )
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
    if( imageMemory )
        vkFreeMemory(device->GetDevice(), imageMemory, nullptr);
    if( image && destroyImage )
        vkDestroyImage(device->GetDevice(), image, nullptr);
    if (resolvedImageMemory)
        vkFreeMemory(device->GetDevice(), resolvedImageMemory, nullptr);
    if (resolvedImage)
        vkDestroyImage(device->GetDevice(), resolvedImage, nullptr);
}

void VulkanTexture::Initialize()
{
    this->CreateImage(VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanTexture::Initialize(VkImageAspectFlags aspectFlags, VkImage acquiredImage)
{
    this->image = acquiredImage;
    destroyImage = false;
    this->CreateImageView(aspectFlags);
}

void VulkanTexture::Initialize(VkImageUsageFlags usage, VkMemoryPropertyFlags memProperties, VkImageAspectFlags aspectFlags)
{
    this->CreateImage(usage, memProperties);
    this->CreateImageView(aspectFlags);
}

void VulkanTexture::CreateImage(VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkFormatFeatureFlags formatFlags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    auto imageType = (dimensions.x == 1 && dimensions.y == 1) ? VK_IMAGE_TYPE_1D : (dimensions.z == 1) ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    auto format = this->device->FindSupportedFormat({ imageFormat }, tiling, formatFlags);
    if (format == VK_FORMAT_UNDEFINED) // if we couldn't get a supported format with optimal tiling then try linear tiling
    {
        tiling = VK_IMAGE_TILING_LINEAR;

        format = this->device->FindSupportedFormat({ imageFormat }, tiling, formatFlags);
        if (format == VK_FORMAT_UNDEFINED)
            throw std::runtime_error("Failed to find supported format.");
    }

    if (generateMipmaps)
    {
        VkImageFormatProperties formatProperties = {};
        auto res = vkGetPhysicalDeviceImageFormatProperties(this->device->GetPhysicalDevice(), imageFormat, imageType, tiling, usage, NULL, &formatProperties);
        if( res != VK_ERROR_FORMAT_NOT_SUPPORTED )
        {
            mipLevels = std::min(mipLevels, formatProperties.maxMipLevels);
            generateMipmaps = mipLevels > 1;
        }else{
            VK_LOG << "vkGetPhysicalDeviceImageFormatProperties returned VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
        }
    }

    auto props = this->device->FindFormatProperties(imageFormat);
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = dimensions.x;
    imageInfo.extent.height = dimensions.y;
    imageInfo.extent.depth = dimensions.z;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = imageFormat;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = sampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image.");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device->GetDevice(), image, &memRequirements);

    device->AllocateMemory(memRequirements, properties, &imageMemory);

    vkBindImageMemory(device->GetDevice(), image, imageMemory, 0);

    // multisampling, create a resolve target
    if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
    {
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &resolvedImage) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image.");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device->GetDevice(), resolvedImage, &memRequirements);

        device->AllocateMemory(memRequirements, properties, &resolvedImageMemory);

        vkBindImageMemory(device->GetDevice(), resolvedImage, resolvedImageMemory, 0);
    }
}

void VulkanTexture::CreateImageView(VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = (dimensions.x == 1 && dimensions.y == 1) ? VK_IMAGE_VIEW_TYPE_1D : (dimensions.z == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    viewCreateInfo.format = imageFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = mipLevels;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device->GetDevice(), &viewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view.");
}

void VulkanTexture::TransitionImageLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange)
{
    VkCommandBuffer commandBuffer = CreateSingleCommandBuffer(device->GetDevice(), device->GetCommandPool());
    this->TransitionImageLayout(srcLayout, dstLayout, subresourceRange, commandBuffer);
    SubmitFreeCommandBuffer(device->GetDevice(), device->GetCommandPool(), device->GetDefaultQueue(), commandBuffer);
}

void VulkanTexture::TransitionImageLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange, VkCommandBuffer commandBuffer)
{
    this->TransitionImageLayout(image, srcLayout, dstLayout, subresourceRange, commandBuffer);
}

void VulkanTexture::TransitionImageLayout(VkImage transitionImage, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange, VkCommandBuffer commandBuffer)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = srcLayout;
    barrier.newLayout = dstLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = transitionImage;

    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (imageFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || imageFormat == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        barrier.subresourceRange = subresourceRange;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    switch (srcLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    switch (dstLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (barrier.srcAccessMask == 0)
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanTexture::CopyBufferToTexture(std::shared_ptr<VulkanBuffer> pixelData)
{
    this->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(this->device, pixelData->GetBuffer(), image, dimensions.x, dimensions.y, dimensions.z);
    if(generateMipmaps)
        this->GenerateMipmaps();
    else
        this->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanTexture::WriteToFile(std::string filename)
{
    auto buffer = this->device->CreateHostBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_FORMAT_UNDEFINED);
    uint32_t bufferSize = this->dimensions.x * this->dimensions.y * this->dimensions.z * 4 /*RGBA*/ * 4 /*32bpp*/;
    buffer->Allocate(bufferSize);
    this->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    CopyImageToBuffer(this->device, buffer->GetBuffer(), this->image, this->dimensions.x, this->dimensions.y, this->dimensions.z);
    this->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    void *pixelData = buffer->Map();
    
    {
        auto imageWidth = this->dimensions.x;
        auto imageHeight = this->dimensions.y;
        FILE* output_image;
        fopen_s(&output_image, filename.c_str(), "wt");
        fprintf(output_image, "P3\n");
        fprintf(output_image, "# \n");
        fprintf(output_image, "%d %d\n", imageWidth, imageHeight);
        fprintf(output_image, "%u\n", (unsigned int)255);

        auto pixels = static_cast<float*>(pixelData);
        size_t l = 0;
        for (unsigned int i = 0; i < imageHeight; ++i)
        {
            for (unsigned int j = 0; j < imageWidth; ++j)
            {
                float* pixel = reinterpret_cast<float*>(&pixels[i * imageWidth + j]);
                fprintf(output_image, "%u ", (unsigned int)(pixel[2]/255.0));
                fprintf(output_image, "%u ", (unsigned int)(pixel[1]/255.0));
                fprintf(output_image, "%u ", (unsigned int)(pixel[0]/255.0));
            }
            fprintf(output_image, "\n");
        }
        fclose(output_image);
    }

    buffer->UnMap();
}

void VulkanTexture::GenerateMipmaps()
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(this->device->GetPhysicalDevice(), imageFormat, &formatProperties);

    if ( !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) )
        throw std::runtime_error("Current texture image format does not support linear blitting.");

    VkCommandBuffer commandBuffer = CreateSingleCommandBuffer(this->device->GetDevice(), this->device->GetCommandPool());

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    glm::ivec2 mipDimensions(dimensions.x, dimensions.y);

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;

        // layout transition from destination to source
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipDimensions.x, mipDimensions.y, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipDimensions.x > 1 ? mipDimensions.x / 2 : 1, mipDimensions.y > 1 ? mipDimensions.y / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // layout transition from source to shader read access
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        mipDimensions.x = mipDimensions.x > 1 ? mipDimensions.x / 2 : mipDimensions.x;
        mipDimensions.y = mipDimensions.y > 1 ? mipDimensions.y / 2 : mipDimensions.y;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    SubmitFreeCommandBuffer(this->device->GetDevice(), this->device->GetCommandPool(), this->device->GetDefaultQueue(), commandBuffer);
}

VkImage VulkanTexture::Resolve(VkCommandBuffer commandBuffer)
{
    VkImageResolve region = {};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = { 0, 0, 0 };
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = { 0, 0, 0 };
    region.extent = { this->dimensions.x, this->dimensions.y, this->dimensions.z };

    this->TransitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, commandBuffer);
    this->TransitionImageLayout(resolvedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, commandBuffer);
    vkCmdResolveImage(commandBuffer, this->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resolvedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    this->TransitionImageLayout(resolvedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, commandBuffer);
    this->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, commandBuffer);

    return resolvedImage;
}
