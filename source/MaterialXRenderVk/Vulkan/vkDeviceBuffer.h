#pragma once
#include "vkBuffer.h"

class VulkanDeviceBuffer : public VulkanBuffer
{
    public:
    VulkanDeviceBuffer(VulkanDevicePtr _device, VkBufferUsageFlags _bufferUsage, VkFormat _bufferFormat);
    virtual ~VulkanDeviceBuffer();

    virtual void Write(void *bufferData, VkDeviceSize bufferDataSize) override;
};
