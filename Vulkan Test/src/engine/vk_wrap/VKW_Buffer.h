#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"

class VKW_Buffer
{
public:
	// treating properties as required
	VKW_Buffer(std::shared_ptr<VKW_Device> device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool mappable);
	~VKW_Buffer();
	
	void copy(const void* data);
	void* map();
	void unmap();
private:
	VmaAllocator allocator;

	VkBuffer buffer;
	size_t size;
	VmaAllocation allocation; // dont peek inside, treat as opaque
	VkDeviceMemory memory;

	bool mappable;
public:
	inline VkBuffer get_buffer() const { return buffer; };
};

