#include "VKW_Buffer.h"

void VKW_Buffer::init(const VKW_Device* vkw_device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo sharing_info, Mapping mapping, const std::string& obj_name)
{
	device = vkw_device;
	allocator = device->get_allocator();
	length = (size_t) size;
	name = obj_name;

	if (size == 0) {
		throw RuntimeException(fmt::format("Tried to allocate buffer {} with size 0", obj_name), __FILE__, __LINE__);
	}

	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = sharing_info.mode;
	if (buffer_info.sharingMode & VK_SHARING_MODE_CONCURRENT) {
		buffer_info.pQueueFamilyIndices = sharing_info.queue_families.data();
		buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(sharing_info.queue_families.size());
	}

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	if (mapping == Mapping::Mapped) {
		// does sequential access as mapped memory should be written to using memcpy
		alloc_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		
	} else if (mapping == Mapping::Persistent) {
		alloc_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT; // directly mapped from beginning
	}
	if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		alloc_create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	VmaAllocationInfo alloc_info;
	VK_CHECK_ET(
		vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer, &allocation, &alloc_info), 
		RuntimeException, 
		fmt::format("Failed to allocate buffer ({})", name)
	);

#ifdef TRACY_ENABLE
	tracy_mem_instance_id = buffer_instance_count++;
	TracyAllocN(reinterpret_cast<void*>(tracy_mem_instance_id), alloc_info.size, "Buffers");
#endif

	memory = alloc_info.deviceMemory;

	// name device
	device->name_object((uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void VKW_Buffer::del()
{
	if (buffer && allocation && memory) {
		vmaDestroyBuffer(allocator, buffer, allocation);

#ifdef TRACY_ENABLE
		TracyFreeN(reinterpret_cast<void*>(tracy_mem_instance_id), "Buffers");
#endif

		buffer = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}
}

void VKW_Buffer::copy_into(const void* data, size_t data_size, size_t offset)
{
	if (offset + data_size > size()) {
		throw RuntimeException(
			fmt::format("Tried copy that would write outside of bounds of buffer ({})", name), __FILE__, __LINE__
		);
	}

	vmaCopyMemoryToAllocation(allocator, data, allocation, offset, data_size);
}

void VKW_Buffer::copy_into(const VKW_CommandPool* command_pool, const VKW_Buffer& other_buffer)
{
	if (size() != other_buffer.size()) {
		throw RuntimeException(
			fmt::format("Tried to copy from a buffer ({}) with different size as dst buffer ({})", other_buffer.name, name),
			__FILE__, __LINE__
		);
	}
	
	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true,
		"Buffer copy CMD"
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

void VKW_Buffer::copy_from(void* data, size_t data_size) const
{
	if (data_size > size()) {
		throw RuntimeException(
			fmt::format("Tried copy that would write outside of bounds of buffer ({})", name), __FILE__, __LINE__
		);
	}

	vmaCopyAllocationToMemory(allocator, allocation, 0, data, data_size);
}

VKW_Buffer create_staging_buffer(const VKW_Device* device, VkDeviceSize buffer_size, const void* data, size_t data_size, const std::string& name)
{
	VKW_Buffer staging_buffer = {};
	staging_buffer.init(
		device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		sharing_exlusive(),
		Mapping::Mapped,
		name
	);

	staging_buffer.copy_into(data, data_size);

	return staging_buffer;
}
