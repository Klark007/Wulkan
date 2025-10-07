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
	const VKW_Device* device = nullptr;
	std::string m_name;
	const VKW_CommandPool* command_pool = nullptr;
	const VKW_Queue* queue = nullptr;

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	bool single_use;
public:
	inline VkCommandBuffer get_command_buffer() const { return command_buffer; };
	inline operator VkCommandBuffer() const { return command_buffer; };

	inline void begin_debug_zone(const std::string& name) const;
	inline void end_debug_zone() const;
};

void VKW_CommandBuffer::begin_debug_zone(const std::string& name) const
{
#ifndef NDEBUG
	VkDebugUtilsLabelEXT label_info{};
	label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label_info.pLabelName = name.c_str();

	vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label_info);
#endif
}

inline void VKW_CommandBuffer::end_debug_zone() const
{
#ifndef NDEBUG
	vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif
}