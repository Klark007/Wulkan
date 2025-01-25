#pragma once
#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_Object.h"

class VKW_DescriptorPool : public VKW_Object
{
public:
	VKW_DescriptorPool() = default;
	// initializes the pool, before that add descriptors to it
	void init(const VKW_Device* device, uint32_t max_sets);
	void reset();
	void del() override;
private:
	const VKW_Device* device;

	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorPoolSize> pool_size;
public:
	void add_type(VkDescriptorType type, uint32_t count);
	// add a layout to the pool size (multiplied by count)
	void add_layout(const class VKW_DescriptorSetLayout& layout, uint32_t count);

	inline VkDescriptorPool get_descriptor_pool() const { return descriptor_pool; };
	inline operator VkDescriptorPool() const { return descriptor_pool; };
};

