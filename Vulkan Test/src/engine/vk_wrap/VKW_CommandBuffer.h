#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandPool.h"

class VKW_CommandBuffer
{
public:
	VKW_CommandBuffer(std::shared_ptr<VKW_Device> device, std::shared_ptr<VKW_CommandPool> command_pool);
private:
	VkCommandBuffer command_buffer;
public:
	inline VkCommandBuffer get_command_buffer() const { return command_buffer; };
};

