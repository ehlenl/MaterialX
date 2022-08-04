#include "vkRenderPass.h"
#include "vkRenderTarget.h"

VulkanRenderPass::VulkanRenderPass(std::shared_ptr<VulkanDevice> _device)
:   device(_device),
    renderPass(nullptr),
    fence(nullptr),
    commandBuffer(nullptr)
{
    //
}

VulkanRenderPass::~VulkanRenderPass()
{
    if(renderPass)
        vkDestroyRenderPass(this->device->GetDevice(), renderPass, nullptr);
    if(fence)
        vkDestroyFence(this->device->GetDevice(), fence, nullptr);
}

void VulkanRenderPass::Initialize(std::shared_ptr<VulkanRenderTarget> _renderTarget, bool createRenderPass)
{
    renderTarget = _renderTarget;
    if (createRenderPass)
    {
        this->CreateRenderPass();
        renderTarget->CreateFrameBuffer(this->GetRenderPass());
    }
    this->CreateCommandBuffers();

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ERROR_CHECK(vkCreateFence(this->device->GetDevice(), &fenceInfo, nullptr, &fence));
}

void VulkanRenderPass::CreateRenderPass()
{
    if(renderPass)
        vkDestroyRenderPass(this->device->GetDevice(), renderPass, nullptr);

    std::vector<VkAttachmentDescription> attachments(1);
    std::vector<VkAttachmentReference> attachmentReferences(1);
    attachmentReferences[0].attachment = 0; // fragment shader out
    attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0] = this->renderTarget->GetColorAttachmentDescription();

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = static_cast<uint32_t>(attachments.size());
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (this->renderTarget->GetDepthTarget() != nullptr)
        attachments.push_back(this->renderTarget->GetDepthAttachmentDescription());

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReferences[0];
    subpass.pDepthStencilAttachment = (this->renderTarget->GetDepthTarget() == nullptr) ? nullptr : &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_ERROR_CHECK(vkCreateRenderPass(device->GetDevice(), &renderPassInfo, nullptr, &renderPass));
}

void VulkanRenderPass::CreateCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->device->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_ERROR_CHECK(vkAllocateCommandBuffers(this->device->GetDevice(), &allocInfo, &commandBuffer));
}

void VulkanRenderPass::BeginRenderPass(uint32_t targetIndex)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = renderTarget->GetFramebuffer(targetIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    auto extent = renderTarget->GetExtent();
    renderPassInfo.renderArea.extent = { extent.x, extent.y };

    std::vector<VkClearValue> clearValues(2);
    //clearValues[0].color = { 0.392f, 0.584f, 0.929f, 1.0f };
    clearValues[0].color = { 0.5372f, 0.5568f, 0.549f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::EndRenderPass()
{
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanRenderPass::Draw()
{
    vkResetFences(this->device->GetDevice(), 1, &fence);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_ERROR_CHECK(vkQueueSubmit(this->device->GetDefaultQueue(), 1, &submitInfo, fence));
    vkWaitForFences(this->device->GetDevice(), 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}