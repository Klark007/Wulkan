#include "VKW_Buffer.h"

void VKW_Buffer::init(const VKW_Device* vkw_device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo sharing_info, bool m)
{
	device = vkw_device;
	allocator = device->get_allocator();
	mappable = m;
	length = (size_t) size;

	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = sharing_info.mode;
	if (buffer_info.sharingMode & VK_SHARING_MODE_CONCURRENT) {
		buffer_info.pQueueFamilyIndices = sharing_info.queue_families.data();
		buffer_info.queueFamilyIndexCount = sharing_info.queue_families.size();
	}

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	if (mappable) {
		// does sequential access as mapped memory should be written to using memcpy
		alloc_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}
	if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		alloc_create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	VmaAllocationInfo alloc_info;
	VK_CHECK_ET(vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer, &allocation, &alloc_info), RuntimeException, "Failed to allocate buffer");
	memory = alloc_info.deviceMemory;
}

void VKW_Buffer::del()
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
	memcpy(addr, data, size());
	unmap();
}

void VKW_Buffer::copy(const VKW_CommandPool* command_pool, const VKW_Buffer& other_buffer)
{
	if (size() != other_buffer.size()) {
		throw RuntimeException("Tried to copy from a buffer with different size as source buffer", __FILE__, __LINE__);
	}
	
	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true
	);
	command_buffer.begin_single_use();


	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size();

	// copy into self
	vkCmdCopyBuffer(command_buffer, other_buffer, buffer, 1, &copyRegion);

	command_buffer.submit_single_use();
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

VKW_Buffer create_staging_buffer(const VKW_Device* device, const void* data, VkDeviceSize size)
{
	VKW_Buffer staging_buffer = {};
	staging_buffer.init(
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		sharing_exlusive(),
		true
	);

	staging_buffer.copy(data);

	return staging_buffer;
}
