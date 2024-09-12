#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"

// ideally want a a dedicated queue for transfer or compute
// Has a queue family that supports one operations but not graphics nor the other.
// If not then use one that is seperate from graphics

class VKW_Queue
{
public:
	VKW_Queue(std::shared_ptr<VKW_Device> device, vkb::QueueType type);
private:
	VkQueue queue;
	uint32_t family_idx;
public:
	inline VkQueue get_queue() const { return queue; };
	inline uint32_t get_queue_family() const { return family_idx; };
};

