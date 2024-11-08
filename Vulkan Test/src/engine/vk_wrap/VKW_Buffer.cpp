#include "VKW_Buffer.h"

VKW_Buffer::VKW_Buffer(std::shared_ptr<VKW_Device> device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool mappable)
	: allocator{device->get_allocator()}, mappable{mappable}, size {(size_t) size}
{
	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size = size;
	buffer_info.usage = usage;
	// TODO: Sharing mode with queue families, currently: implicit

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	alloc_create_info.requiredFlags = properties;
	if (mappable) {
		// does sequential access as mapped memory should be written to using memcpy
		alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}

	VmaAllocationInfo alloc_info;
	VK_CHECK_ET(vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer, &allocation, &alloc_info), RuntimeException, "Failed to allocate buffer");
	memory = alloc_info.deviceMemory;
}

VKW_Buffer::~VKW_Buffer()
{
	if (buffer && allocation && memory) {
		vmaDestroyBuffer(allocator, buffer, allocation);
		buffer = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}
}

void VKW_Buffer::copy(const void* data)
{
	void* addr = map();
	memcpy(addr, data, size);
	unmap();
}

// maps whole buffer to adress and returns address
void* VKW_Buffer::map()
{
	if (!mappable) {
		throw SetupException("Tried to map a buffer that was not created as mappable", __FILE__, __LINE__);
	}

	void* data;

	if (vmaMapMemory(allocator, allocation, &data)) {
		throw RuntimeException("Failed to map buffer to memory", __FILE__, __LINE__);
	}
	
	return data;
}

void VKW_Buffer::unmap()
{
	if (!mappable) {
		throw SetupException("Tried to unmap a buffer that was not created as mappable", __FILE__, __LINE__);
	}

	vmaUnmapMemory(allocator, allocation);
}
