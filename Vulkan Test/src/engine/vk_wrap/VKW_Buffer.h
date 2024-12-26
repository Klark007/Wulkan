#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandBuffer.h"


class VKW_Buffer
{
public:
	// treating properties as required
	VKW_Buffer(std::shared_ptr<VKW_Device> device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo info, bool mappable);
	~VKW_Buffer();
	
	void copy(const void* data); // copies data into VKW_Buffer
	void copy(std::shared_ptr<VKW_CommandPool> command_pool, const std::shared_ptr<VKW_Buffer> other_buffer); // copies other buffer into this one, creates and submits single use command buffer
	void* map(); // maps buffer into cpu accessible memory and returns pointer to it 
	void unmap();
private:
	std::shared_ptr<VKW_Device> device;

	VmaAllocator allocator;

	VkBuffer buffer;
	size_t length;
	VmaAllocation allocation; // dont peek inside, treat as opaque
	VkDeviceMemory memory;

	bool mappable;
public:
	inline VkBuffer get_buffer() const { return buffer; };
	inline operator VkBuffer() const { return buffer; };
	inline size_t size() const { return length; };
};

std::shared_ptr<VKW_Buffer> create_staging_buffer(std::shared_ptr<VKW_Device> device, void* data, VkDeviceSize size);