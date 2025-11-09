#include "VKW_Buffer.h"
#include <spdlog/spdlog.h>

void VKW_Buffer::init(const VKW_Device* vkw_device, VkDeviceSize size, VkBufferUsageFlags usage, SharingInfo sharing_info, bool m, const std::string& obj_name)
{
	device = vkw_device;
	allocator = device->get_allocator();
	mappable = m;
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
	if (mappable) {
		// does sequential access as mapped memory should be written to using memcpy
		alloc_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
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
	m_memory_offset = alloc_info.offset;

	// name device
	device->name_object((uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void VKW_Buffer::del()
{
	if (buffer && allocation && memory) {
		// if mapped need to unmap before destroy
		if (is_mapped) {
			unmap();
		}
		vmaDestroyBuffer(allocator, buffer, allocation);

#ifdef TRACY_ENABLE
		TracyFreeN(reinterpret_cast<void*>(tracy_mem_instance_id), "Buffers");
#endif

		buffer = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}
}

void VKW_Buffer::copy(const void* data, size_t data_size, size_t offset)
{
	if (offset + data_size > size()) {
		throw RuntimeException(
			fmt::format("Tried copy that would write outside of bounds of buffer ({})", name), __FILE__, __LINE__
		);
	}
	memcpy_s((char*) mapped_address+offset, size(), data, data_size);
}

void VKW_Buffer::copy(const VKW_CommandPool* command_pool, const VKW_Buffer& other_buffer)
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

void VKW_Buffer::flush()
{
	uint32_t nonCoherentAtomSize = device->get_device_properties().limits.nonCoherentAtomSize;

	uint32_t alignedSize = (length - 1) - ((length - 1) % nonCoherentAtomSize) + nonCoherentAtomSize;

	VkMappedMemoryRange memory_range{
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.memory = memory,
		.offset = m_memory_offset, // TODO is this correct
		.size = alignedSize
	};
	
	VK_CHECK_ET(
		vkFlushMappedMemoryRanges(*device, 1, &memory_range), 
		RuntimeException, 
		fmt::format("Failed to allocate buffer ({})", name)
	);
}

// maps whole buffer to cpu adress
void VKW_Buffer::map()
{
	if (!mappable) {
		throw SetupException(fmt::format("Tried to map a buffer ({}) that was not created as mappable", name), __FILE__, __LINE__);
	}

	VK_CHECK_ET(vmaMapMemory(allocator, allocation, &mapped_address), RuntimeException, fmt::format("Failed to map buffer ({}) to memory", name));
	is_mapped = true;
}

void VKW_Buffer::unmap()
{
	if (!mappable) {
		throw SetupException(fmt::format("Tried to unmap a buffer ({}) that was not created as mappable", name), __FILE__, __LINE__);
	}

	mapped_address = nullptr;
	vmaUnmapMemory(allocator, allocation);
	is_mapped = false;
}

VKW_Buffer create_staging_buffer(const VKW_Device* device, VkDeviceSize buffer_size, const void* data, size_t data_size, const std::string& name)
{
	VKW_Buffer staging_buffer = {};
	staging_buffer.init(
		device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		sharing_exlusive(),
		true,
		name
	);

	staging_buffer.map();
	staging_buffer.copy(data, data_size);
	staging_buffer.unmap();

	return staging_buffer;
}
