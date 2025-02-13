#include "VKW_CommandPool.h"

void VKW_CommandPool::init(const VKW_Device* vkw_device, const VKW_Queue* vkw_queue, VkCommandPoolCreateFlags flags, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;
	queue = vkw_queue;

	VkCommandPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = flags;
	create_info.queueFamilyIndex = queue->get_queue_family();

	VK_CHECK_ET(vkCreateCommandPool(*device, &create_info, nullptr, &command_pool), SetupException, std::format("Failed to create command pool ({})", name));
	device->name_object((uint64_t) command_pool, VK_OBJECT_TYPE_COMMAND_POOL, name);
}

void VKW_CommandPool::del()
{
	// Carefull: automatically frees all command buffers allocated from this pool
	VK_DESTROY(command_pool, vkDestroyCommandPool, *device, command_pool);
}
