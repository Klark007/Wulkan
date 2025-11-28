#pragma once

#include "VKW_Object.h"

#include "VKW_Device.h"
#include "VKW_Queue.h"

class VKW_CommandPool : public VKW_Object
{
public:
	VKW_CommandPool() = default;
	void init(const VKW_Device* vkw_device, const VKW_Queue* vkw_queue, const std::string& obj_name);

	void reset() const;

	void del() override;
private:
	const VKW_Device* device = nullptr;
	std::string name;
	const VKW_Queue* queue = nullptr;

	VkCommandPool command_pool = VK_NULL_HANDLE;
public:
	inline VkCommandPool get_command_pool() const { return command_pool; };
	inline operator VkCommandPool() const { return command_pool; };
	inline const VKW_Queue* get_queue() const { return queue; };
};
