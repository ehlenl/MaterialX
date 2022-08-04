#include "vkSwapchain.h"
#include "vkRenderPass.h"
#include "vkTexture.h"
#include "vkHelpers.h"
#include "vkRenderTarget.h"

#include <algorithm>

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDevice> _device, const glm::uvec2 &extent)
:   swapchain(nullptr),
    device(_device),
    imageCount(2),
    currentFrame(0),
    blit(false)
{
    swapchainMutex.store(0);

    renderTarget = std::make_shared<VulkanRenderTarget>(device, glm::uvec3(extent, 1u));

    VulkanDevice::SwapChainSupportDetails swapChainSupport;
    device->QuerySwapChainSupport(swapChainSupport);
    swapchainExtent.width = std::max(swapChainSupport.capabilities.minImageExtent.width, std::min(swapChainSupport.capabilities.maxImageExtent.width, extent.x));
    swapchainExtent.height = std::max(swapChainSupport.capabilities.minImageExtent.height, std::min(swapChainSupport.capabilities.maxImageExtent.height, extent.y));

    this->CreateSwapchain();
    this->CreateSemaphores();
}

VulkanSwapchain::~VulkanSwapchain()
{
    for( size_t i=0; i<imageCount; ++i )
    {
        if( imageAvailableSemaphores[i] != nullptr )
        {
            vkDestroySemaphore(this->device->GetDevice(), imageAvailableSemaphores[i], nullptr);
            imageAvailableSemaphores[i] = nullptr;
        }
        if( renderFinishedSemaphores[i] != nullptr )
        {
            vkDestroySemaphore(this->device->GetDevice(), renderFinishedSemaphores[i], nullptr);
            renderFinishedSemaphores[i] = nullptr;
        }
        if( inFlightFences[i] != nullptr )
        {
            vkDestroyFence(this->device->GetDevice(), inFlightFences[i], nullptr);
            inFlightFences[i] = nullptr;
        }
    }
    if( swapchain )
        vkDestroySwapchainKHR(this->device->GetDevice(), swapchain, nullptr);
}

void VulkanSwapchain::CreateSwapchain()
{
    VulkanDevice::SwapChainSupportDetails swapChainSupport;
    device->QuerySwapChainSupport(swapChainSupport);

    VkSurfaceFormatKHR surfaceFormat = device->FindSurfaceFormat(swapChainSupport.formats);

    // find best present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : swapChainSupport.presentModes)
    {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = availablePresentMode;
            break;
        }
        else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            presentMode = availablePresentMode;
    }

    uint32_t maxImageSupport = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && maxImageSupport > swapChainSupport.capabilities.maxImageCount)
        maxImageSupport = swapChainSupport.capabilities.maxImageCount;
    if( imageCount > maxImageSupport )
        throw std::runtime_error("Requested image count " + std::to_string(imageCount) + " exceeds device capability of maximum " + std::to_string(maxImageSupport) + " images.");

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device->GetSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_ERROR_CHECK( vkCreateSwapchainKHR(device->GetDevice(), &createInfo, nullptr, &swapchain) );

    // get all of the images for this swapchain
    vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &imageCount, images.data());

    swapchainImageFormat = surfaceFormat.format;

    auto swapchainDimensions = glm::uvec3(swapchainExtent.width, swapchainExtent.height, 1u);

    for( size_t i=0; i<imageCount; ++i )
    {
        auto &image = images[i];
        auto colorTarget = std::make_shared<VulkanTexture>(device, swapchainImageFormat, swapchainDimensions, false);
        colorTarget->Initialize(VK_IMAGE_ASPECT_COLOR_BIT, image);
        renderTarget->AddColorTarget(colorTarget);
    }

    
    VkFormat depthFormat = device->FindSupportedDepthFormat();
    auto depthTexture = std::make_shared<VulkanTexture>(device, depthFormat, swapchainDimensions, false);
    depthTexture->Initialize(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    depthTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    renderTarget->SetDepthTarget(depthTexture);

    blitCommandBuffers.resize(this->imageCount);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->device->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(blitCommandBuffers.size());

    VK_ERROR_CHECK(vkAllocateCommandBuffers(this->device->GetDevice(), &allocInfo, blitCommandBuffers.data()));
}

void VulkanSwapchain::CreateSemaphores()
{
    imageAvailableSemaphores.resize(imageCount);
    renderFinishedSemaphores.resize(imageCount);
    inFlightFences.resize(imageCount);
    for( size_t i=0; i<imageCount; ++i )
    {
        imageAvailableSemaphores[i] = nullptr;
        renderFinishedSemaphores[i] = nullptr;
        inFlightFences[i]=nullptr;
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for( size_t i=0; i<imageCount; ++i )
    {
        VK_ERROR_CHECK(vkCreateFence(this->device->GetDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
        VK_ERROR_CHECK(vkCreateSemaphore(this->device->GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
        VK_ERROR_CHECK(vkCreateSemaphore(this->device->GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
    }
}

void VulkanSwapchain::Bind(std::shared_ptr<VulkanTexture> srcTexture)
{
    swapchainMutex.store(1);
    vkQueueWaitIdle(this->device->GetDefaultQueue()); // ensure current commands have completed

    if (srcTexture)
    {
        bool msaa = srcTexture->SampleCount() != VK_SAMPLE_COUNT_1_BIT;
        blit = true;
        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        for (uint32_t i = 0; i < this->imageCount; ++i)
        {
            auto& blitCommandBuffer = blitCommandBuffers[i];

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            VK_ERROR_CHECK(vkBeginCommandBuffer(blitCommandBuffer, &beginInfo));

            auto targetTexture = this->renderTarget->GetColorTarget(i);
            
            targetTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, blitCommandBuffer);

            
            VkImage srcImage = srcTexture->GetImage();
            if(msaa)
                srcImage = srcTexture->Resolve(blitCommandBuffer);
            else
                srcTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, blitCommandBuffer);

            // Copy srcTexture image to swap chain image
            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { static_cast<int>(this->swapchainExtent.width), static_cast<int>(this->swapchainExtent.height), 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = 0;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { static_cast<int>(this->swapchainExtent.width), static_cast<int>(this->swapchainExtent.height), 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(blitCommandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            if(!msaa)
                srcTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange, blitCommandBuffer);
            
            // Transition swap chain image back for presentation
            targetTexture->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresourceRange, blitCommandBuffer);
            VK_ERROR_CHECK(vkEndCommandBuffer(blitCommandBuffer));
        }
    }
    else {
        blit = false;
    }
    swapchainMutex.store(0);
}

void VulkanSwapchain::Blit()
{
    if (swapchainMutex.load() == 1)
        return;
    
    if (blit)
    {
        vkWaitForFences(this->device->GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(this->device->GetDevice(), 1, &inFlightFences[currentFrame]);
    }

    uint32_t imageIndex;
    vkAcquireNextImageKHR(this->device->GetDevice(), this->swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkSemaphore> presentWaitSemaphores;
    if (blit)
    {
        std::vector<VkSemaphore> blitWaitSemaphores = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(blitWaitSemaphores.size());
        submitInfo.pWaitSemaphores = blitWaitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &blitCommandBuffers[imageIndex];

        presentWaitSemaphores.push_back(renderFinishedSemaphores[currentFrame]);
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(presentWaitSemaphores.size());
        submitInfo.pSignalSemaphores = presentWaitSemaphores.data();

        VK_ERROR_CHECK(vkQueueSubmit(this->device->GetDefaultQueue(), 1, &submitInfo, inFlightFences[currentFrame]));
        vkWaitForFences(this->device->GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    }
    else {
        presentWaitSemaphores = { imageAvailableSemaphores[currentFrame] };
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(presentWaitSemaphores.size());
    presentInfo.pWaitSemaphores = presentWaitSemaphores.data();

    VkSwapchainKHR swapChains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(this->device->GetDefaultQueue(), &presentInfo);
    
    currentFrame = (currentFrame + 1) % imageCount;
}

glm::uvec2 VulkanSwapchain::GetExtent()
{
    glm::uvec3 extent = this->renderTarget->GetExtent();
    return glm::uvec2(extent.x, extent.y);
}

std::shared_ptr<VulkanTexture> VulkanSwapchain::GetCurrentImage()
{
    return this->renderTarget->GetColorTarget(currentFrame);
}