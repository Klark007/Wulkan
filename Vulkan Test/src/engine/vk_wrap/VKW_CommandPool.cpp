#include "VKW_CommandPool.h"

VKW_CommandPool::VKW_CommandPool(std::shared_ptr<VKW_Device> vkw_device, std::shared_ptr<VKW_Queue> vkw_queue, VkCommandPoolCreateFlags flags)
	: device {vkw_device}, queue {vkw_queue}
{
	VkCommandPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = flags;
	create_info.queueFamilyIndex = queue->get_queue_family();

	VK_CHECK_ET(vkCreateCommandPool(device->get_device(), &create_info, nullptr, &command_pool), SetupException, "Failed to create Command pool");
}

VKW_CommandPool::~VKW_CommandPool()
{
	// Carefull: automatically frees all command buffers allocated from this pool
	VK_DESTROY(command_pool, vkDestroyCommandPool, device->get_device(), command_pool);
}
