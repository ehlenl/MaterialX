#include "vkHostBuffer.h"
#include "vkHelpers.h"
#include <cstring>

VulkanHostBuffer::VulkanHostBuffer(std::shared_ptr<VulkanDevice> _device, VkBufferUsageFlags _bufferUsage, VkFormat _bufferFormat)
: VulkanBuffer(_device, _bufferUsage, _bufferFormat)
{
    memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

VulkanHostBuffer::~VulkanHostBuffer()
{
    //
}

void VulkanHostBuffer::Write(void *bufferData, VkDeviceSize bufferDataSize)
{
    if( bufferDataSize != bufferSize )
    {
        if( bufferMemory )
            vkFreeMemory(this->device->GetDevice(), bufferMemory, nullptr);
        if( buffer )
            vkDestroyBuffer(this->device->GetDevice(), buffer, nullptr);

        bufferSize = bufferDataSize;
        this->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage, memoryPropertyFlags, bufferSize, buffer, bufferMemory);
    }

    void* mappedMemory = nullptr;
    vkMapMemory(this->device->GetDevice(), bufferMemory, 0, bufferAllocationSize, 0, &mappedMemory);
    memcpy(mappedMemory, bufferData, bufferSize);
    vkUnmapMemory(this->device->GetDevice(), bufferMemory);
}

void *VulkanHostBuffer::Map()
{
    void* mappedMemory = nullptr;
    vkMapMemory(this->device->GetDevice(), bufferMemory, 0, bufferAllocationSize, 0, &mappedMemory);
    return mappedMemory;
}

void VulkanHostBuffer::UnMap()
{
    vkUnmapMemory(this->device->GetDevice(), bufferMemory);
}
