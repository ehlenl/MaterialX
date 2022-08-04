#include "vkRenderTarget.h"
#include "vkTexture.h"

VulkanRenderTarget::VulkanRenderTarget(std::shared_ptr<VulkanDevice> _device, glm::uvec3 _extent)
: device(_device), extent(_extent)
{
}

VulkanRenderTarget::VulkanRenderTarget(std::shared_ptr<VulkanDevice> _device, glm::uvec3 _extent, std::vector<VkSurfaceFormatKHR> requestedSurfaceFormat)
: device(_device), extent(_extent)
{
    requestedSurfaceFormats = requestedSurfaceFormat;
}

VulkanRenderTarget::~VulkanRenderTarget()
{
    for( auto frameBuffer : frameBuffers )
        vkDestroyFramebuffer(this->device->GetDevice(), frameBuffer, nullptr);
}

void VulkanRenderTarget::CreateFrameBuffer(VkRenderPass renderPass)
{
    for (auto& colorTexture : colorTextures)
    {
        std::vector<VkImageView> attachments;
        attachments.push_back(colorTexture->GetImageView());

        if (depthTexture)
            attachments.push_back(depthTexture->GetImageView());

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = (uint32_t)attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = this->extent.x;
        framebufferInfo.height = this->extent.y;
        framebufferInfo.layers = this->extent.z;

        frameBuffers.resize(frameBuffers.size() + 1);
        VK_ERROR_CHECK(vkCreateFramebuffer(this->device->GetDevice(), &framebufferInfo, nullptr, &frameBuffers[frameBuffers.size()-1]));
    }
}

void VulkanRenderTarget::AddColorTarget(std::shared_ptr<VulkanTexture> colorTarget)
{
    if (colorTextures.size() == 0)
    {
        colorAttachmentDescription = {}; // single color attachment
        colorAttachmentDescription.format = colorTarget->TextureFormat();
        colorAttachmentDescription.samples = colorTarget->SampleCount();
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    colorTextures.push_back(colorTarget);
}

void VulkanRenderTarget::SetDepthTarget(std::shared_ptr<VulkanTexture> depthTarget)
{
    depthTexture = depthTarget;

    depthAttachmentDescription = {};
    depthAttachmentDescription.format = device->FindSupportedDepthFormat();
    depthAttachmentDescription.samples = depthTexture->SampleCount();
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}
