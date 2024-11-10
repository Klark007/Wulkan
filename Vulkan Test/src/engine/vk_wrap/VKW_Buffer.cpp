#include "VKW_Buffer.h"

VKW_Buffer::VKW_Buffer(std::shared_ptr<VKW_Device> device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, SharingInfo sharing_info, bool mappable)
	: allocator{device->get_allocator()}, mappable{mappable}, size {(size_t) size}
{
	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = sharing_info.mode;
	if (buffer_info.sharingMode == VK_SHARING_MODE_CONCURRENT) {
		buffer_info.pQueueFamilyIndices = sharing_info.queue_families.data();
		buffer_info.queueFamilyIndexCount = sharing_info.queue_families.size();
	}

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

void VKW_Buffer::copy(std::shared_ptr<VKW_CommandBuffer> command_buffer, const std::shared_ptr<VKW_Buffer> other_buffer)
{
	if (size != other_buffer->get_size()) {
		throw RuntimeException("Tried to copy from a buffer with different size as source buffer", __FILE__, __LINE__);
	}

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(*command_buffer, buffer, *other_buffer, 1, &copyRegion);
}

// maps whole buffer to adress and returns address
void* VKW_Buffer::map()
{
	if (!mappable) {
		throw SetupException("Tried to map a buffer that was not created as mappable", __FILE__, __LINE__);
	}

	void* data;

	VK_CHECK_ET(vmaMapMemory(allocator, allocation, &data), RuntimeException, "Failed to map buffer to memory");
	
	return data;
}

void VKW_Buffer::unmap()
{
	if (!mappable) {
		throw SetupException("Tried to unmap a buffer that was not created as mappable", __FILE__, __LINE__);
	}

	vmaUnmapMemory(allocator, allocation);
}

std::shared_ptr<VKW_Buffer> create_staging_buffer(std::shared_ptr<VKW_Device> device, void* data, VkDeviceSize size)
{
	std::shared_ptr<VKW_Buffer> staging_buffer = std::make_shared<VKW_Buffer>(
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sharing_exlusive(),
		true
	);

	staging_buffer->copy(data);

	return staging_buffer;
}
