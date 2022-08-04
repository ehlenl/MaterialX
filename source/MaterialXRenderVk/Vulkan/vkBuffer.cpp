#include "vkBuffer.h"
#include "vkHelpers.h"
#include <cstring>

VulkanBuffer::VulkanBuffer(std::shared_ptr<VulkanDevice> _device, VkBufferUsageFlags _bufferUsage, VkFormat _bufferFormat)
:   device(_device),
    bufferUsage(_bufferUsage),
    buffer(nullptr),
    bufferMemory(nullptr),
    bufferSize(0),
    bufferFormat(_bufferFormat)
{
    //
}

VulkanBuffer::~VulkanBuffer()
{
    if( bufferMemory )
        vkFreeMemory(this->device->GetDevice(), bufferMemory, nullptr);
    if( buffer )
        vkDestroyBuffer(this->device->GetDevice(), buffer, nullptr);
}

void VulkanBuffer::CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceSize bufferDataSize, VkBuffer &buf, VkDeviceMemory &bufMemory)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferDataSize; 
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_ERROR_CHECK(vkCreateBuffer(this->device->GetDevice(), &bufferCreateInfo, nullptr, &buf));

    // Allocate buffer memory
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(this->device->GetDevice(), buf, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    bufferAllocationSize = allocateInfo.allocationSize;

    VkMemoryAllocateFlagsInfo allocateFlagsInfo = {};
    allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    allocateInfo.pNext = &allocateFlagsInfo;

    allocateInfo.memoryTypeIndex = this->device->FindMemoryType(memoryRequirements.memoryTypeBits, props);

    VK_ERROR_CHECK(vkAllocateMemory(this->device->GetDevice(), &allocateInfo, NULL, &bufMemory));
    
    // Bind buffer and allocated memory
    VK_ERROR_CHECK(vkBindBufferMemory(this->device->GetDevice(), buf, bufMemory, 0));
}