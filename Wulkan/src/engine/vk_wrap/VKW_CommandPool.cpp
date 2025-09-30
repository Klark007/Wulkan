#include "VKW_CommandPool.h"

void VKW_CommandPool::init(const VKW_Device* vkw_device, const VKW_Queue* vkw_queue, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;
	queue = vkw_queue;

	VkCommandPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // short lived command buffers (as we don't pre record, this is the case); Could use this to enable per command buffer resets but these are inefficient compared to reseting command pools
	create_info.queueFamilyIndex = queue->get_queue_family();

	VK_CHECK_ET(vkCreateCommandPool(*device, &create_info, nullptr, &command_pool), SetupException, fmt::format("Failed to create command pool ({})", name));
	device->name_object((uint64_t) command_pool, VK_OBJECT_TYPE_COMMAND_POOL, name);
}

void VKW_CommandPool::del()
{
	// Carefull: automatically frees all command buffers allocated from this pool
	VK_DESTROY(command_pool, vkDestroyCommandPool, *device, command_pool);
}

void VKW_CommandPool::reset() const
{
	VK_CHECK_ET(vkResetCommandPool(*device, command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT), RuntimeException, "Failed to reset command pool");
}