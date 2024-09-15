#include "VKW_CommandPool.h"

VKW_CommandPool::VKW_CommandPool(std::shared_ptr<VKW_Device> vkw_device, std::shared_ptr<VKW_Queue> queue, VkCommandPoolCreateFlags flags)
	: device {vkw_device}
{
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.queueFamilyIndex = queue->get_queue_family();

	VK_CHECK_ET(vkCreateCommandPool(device->get_device(), &createInfo, nullptr, &command_pool), SetupException, "Failed to create Command pool");
}

VKW_CommandPool::~VKW_CommandPool()
{
	// Carefull: automatically frees all command buffers allocated from this pool
	VK_DESTROY(command_pool, vkDestroyCommandPool, device->get_device(), command_pool);
}
