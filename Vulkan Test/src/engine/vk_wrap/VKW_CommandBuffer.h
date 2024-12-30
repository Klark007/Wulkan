#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandPool.h"

class VKW_CommandBuffer
{
public:
	VKW_CommandBuffer() = default;
	void init(const VKW_Device* vkw_device, const VKW_CommandPool* vkw_command_pool, bool single_use);

	void begin_single_use();
	// submits command buffer and waits for the queue to be idle. WARNING: could take long
	void submit_single_use();
private:
	const VKW_Device* device;
	const VKW_CommandPool* command_pool;
	const VKW_Queue* queue;

	VkCommandBuffer command_buffer;
	bool single_use;
public:
	inline VkCommandBuffer get_command_buffer() const { return command_buffer; };
	inline operator VkCommandBuffer() const { return command_buffer; };
};

