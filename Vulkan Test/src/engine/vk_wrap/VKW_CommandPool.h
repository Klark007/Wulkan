#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_Queue.h"

class VKW_CommandPool
{
public:
	VKW_CommandPool(std::shared_ptr<VKW_Device> vkw_device, std::shared_ptr<VKW_Queue> queue, VkCommandPoolCreateFlags flags);
	~VKW_CommandPool();
private:
	std::shared_ptr<VKW_Device> device;
	VkCommandPool command_pool;
public:
	inline VkCommandPool get_command_pool() const { return command_pool; };
};

