#pragma once
#include "vkDevice.h"

class VulkanBuffer
{
    public:
    VulkanBuffer(std::shared_ptr<VulkanDevice> _device, VkBufferUsageFlags _bufferUsage, VkFormat _bufferFormat);
    virtual ~VulkanBuffer();

    virtual void Write(void *bufferData, VkDeviceSize bufferDataSize) = 0;

    VkDeviceSize GetBufferSize(){ return bufferSize; }
    VkBuffer &GetBuffer(){ return buffer; }
    VkFormat GetBufferFormat(){ return bufferFormat; }

    VkDeviceAddress GetDeviceAddress()
    {
        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = buffer;
        return vkGetBufferDeviceAddress(device->GetDevice(), &addressInfo);
    }

    void Allocate(VkDeviceSize size)
    {
        if (bufferSize == size && bufferMemory && buffer)
            return;

        bufferSize = size;

        if (bufferMemory)
            vkFreeMemory(this->device->GetDevice(), bufferMemory, nullptr);
        if (buffer)
            vkDestroyBuffer(this->device->GetDevice(), buffer, nullptr);

        this->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage, memoryPropertyFlags, bufferSize, buffer, bufferMemory);
    }

    protected:
    std::shared_ptr<VulkanDevice> device;

    void CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceSize bufferSize, VkBuffer &buf, VkDeviceMemory &bufMemory);

    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    VkBufferUsageFlags bufferUsage;
    VkFormat bufferFormat;
    VkMemoryPropertyFlags memoryPropertyFlags;

    VkDeviceSize bufferAllocationSize;
    VkDeviceSize bufferSize;
};
