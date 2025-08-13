#pragma once

#include "../common.h"

#include "VKW_Device.h"
#include "VKW_CommandPool.h"

class VKW_CommandBuffer
{
public:
	VKW_CommandBuffer() = default;
	void init(const VKW_Device* vkw_device, const VKW_CommandPool* vkw_command_pool, bool single_use, const std::string& obj_name);

	void begin_single_use();
	// submits command buffer and waits for the queue to be idle. WARNING: could take long
	void submit_single_use();

	// begins command buffer
	void begin() const;

	// ends and submits the command buffer (with given signal, wait semaphores and fence)
	void submit(const std::vector<VkSemaphore>& wait_semaphores, const std::vector<VkPipelineStageFlags>& wait_stages, const std::vector<VkSemaphore>& signal_semaphores, VkFence fence) const;
private:
	const VKW_Device* device;
	std::string name;
	const VKW_CommandPool* command_pool;
	const VKW_Queue* queue;

	VkCommandBuffer command_buffer;
	bool single_use;
public:
	inline VkCommandBuffer get_command_buffer() const { return command_buffer; };
	inline operator VkCommandBuffer() const { return command_buffer; };
};