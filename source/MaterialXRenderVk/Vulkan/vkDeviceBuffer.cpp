#include "vkDeviceBuffer.h"
#include "vkHelpers.h"
#include <cstring>

VulkanDeviceBuffer::VulkanDeviceBuffer(std::shared_ptr<VulkanDevice> _device, VkBufferUsageFlags _bufferUsage, VkFormat _bufferFormat)
: VulkanBuffer(_device, _bufferUsage, _bufferFormat)
{
    memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

VulkanDeviceBuffer::~VulkanDeviceBuffer()
{
    //
}

void VulkanDeviceBuffer::Write(void *bufferData, VkDeviceSize bufferDataSize)
{
    if( bufferMemory )
        vkFreeMemory(this->device->GetDevice(), bufferMemory, nullptr);
    if( buffer )
        vkDestroyBuffer(this->device->GetDevice(), buffer, nullptr);

    bufferSize = bufferDataSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    this->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, stagingBuffer, stagingMemory);

    void *mappedMemory = nullptr;
    vkMapMemory(this->device->GetDevice(), stagingMemory, 0, bufferSize, 0, &mappedMemory);
    std::memcpy(mappedMemory, bufferData, static_cast<size_t>(bufferSize));
    vkUnmapMemory(this->device->GetDevice(), stagingMemory);

    this->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage, memoryPropertyFlags, bufferSize, buffer, bufferMemory);

    auto commandBuffer = CreateSingleCommandBuffer(this->device->GetDevice(), this->device->GetCommandPool());

    VkBufferCopy copyRegion = {};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

    SubmitFreeCommandBuffer(this->device->GetDevice(), this->device->GetCommandPool(), this->device->GetDefaultQueue(), commandBuffer);

    vkFreeMemory(this->device->GetDevice(), stagingMemory, nullptr);
    vkDestroyBuffer(this->device->GetDevice(), stagingBuffer, nullptr);
}