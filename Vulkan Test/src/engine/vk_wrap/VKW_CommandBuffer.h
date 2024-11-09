#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandPool.h"

class VKW_CommandBuffer
{
public:
	VKW_CommandBuffer(std::shared_ptr<VKW_Device> vkw_device, std::shared_ptr<VKW_CommandPool> vkw_command_pool, bool single_use);

	void begin_single_use();
	void submit_single_use();
private:
	std::shared_ptr<VKW_Device> device;
	std::shared_ptr<VKW_CommandPool> command_pool;
	std::shared_ptr<VKW_Queue> queue;

	VkCommandBuffer command_buffer;
	bool single_use;
public:
	inline VkCommandBuffer get_command_buffer() const { return command_buffer; };
};

