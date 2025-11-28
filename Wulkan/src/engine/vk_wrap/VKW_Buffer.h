#pragma once

#include "VKW_Object.h"

#include "VKW_Device.h"
#include "VKW_CommandBuffer.h"


enum class Mapping {
	NotMapped,
	Mapped,
	Persistent
};

class VKW_Buffer : public VKW_Object
{
public:
	VKW_Buffer() = default;
	// treating properties as required
	void init(const VKW_Device* device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo info, Mapping mapping, const std::string& obj_name);
	void del() override;

	void copy_into(const void* data, size_t data_size, size_t offset=0); // copies data into VKW_Buffer (copies data_size many bytes from data to mapped_address + offset)
	void copy_into(const VKW_CommandPool* command_pool, const VKW_Buffer& other_buffer); // copies other buffer into this one, creates and submits single use command buffer
	void copy_from(void* data, size_t data_size) const; // copies from buffer into data (data_size many bytes)
private:
	const VKW_Device* device = nullptr;
	std::string name;

	VmaAllocator allocator = VK_NULL_HANDLE;

	VkBuffer buffer{};
	size_t length;
	VmaAllocation allocation = VK_NULL_HANDLE; // dont peek inside, treat as opaque
	VkDeviceMemory memory;

#ifdef TRACY_ENABLE
	uint32_t tracy_mem_instance_id;
	inline static uint32_t buffer_instance_count = 0;
#endif
public:
	inline VkBuffer get_buffer() const { return buffer; };
	inline operator VkBuffer() const { return buffer; };
	// size of underlying VkBuffer in bytes
	inline size_t size() const { return length; };
};

VKW_Buffer create_staging_buffer(const VKW_Device* device, VkDeviceSize buffer_size, const void* data, size_t data_size, const std::string& name="Staging buffer");