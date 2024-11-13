#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandBuffer.h"

struct SharingInfo {
	VkSharingMode mode;
	std::vector<uint32_t> queue_families;
};

class VKW_Buffer
{
public:
	// treating properties as required
	VKW_Buffer(std::shared_ptr<VKW_Device> device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo info, bool mappable);
	~VKW_Buffer();
	
	void copy(const void* data); // copies data into VKW_Buffer
	void copy(std::shared_ptr<VKW_CommandBuffer> command_buffer, const std::shared_ptr<VKW_Buffer> other_buffer); // copies other buffer into this one, expects them to be the same size, needs to be in command buffer session 
	void* map(); // maps buffer into cpu accessible memory and returns pointer to it 
	void unmap();
private:
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

inline constexpr SharingInfo sharing_exlusive() {
	return { VK_SHARING_MODE_EXCLUSIVE, {} };
};

std::shared_ptr<VKW_Buffer> create_staging_buffer(std::shared_ptr<VKW_Device> device, void* data, VkDeviceSize size);