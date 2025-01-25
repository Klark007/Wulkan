#include "VKW_DescriptorPool.h"

#include "VKW_DescriptorSet.h"

void VKW_DescriptorPool::init(const VKW_Device* vkw_device, uint32_t max_sets)
{
	device = vkw_device;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allows vkFreeDescriptorSets (done by imgui)

	pool_info.pPoolSizes = pool_size.data();
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());

	pool_info.maxSets = max_sets;

	VK_CHECK_ET(vkCreateDescriptorPool(*device, &pool_info, VK_NULL_HANDLE, &descriptor_pool), RuntimeException, "Failed to create descriptor pool");
}

void VKW_DescriptorPool::reset()
{
	vkResetDescriptorPool(*device, descriptor_pool, 0);
}

void VKW_DescriptorPool::del()
{
	VK_DESTROY(descriptor_pool, vkDestroyDescriptorPool, *device, descriptor_pool);
}

void VKW_DescriptorPool::add_type(VkDescriptorType type, uint32_t count)
{
	VkDescriptorPoolSize size{};
	size.type = type;
	size.descriptorCount = count;

	pool_size.push_back(size);
}

void VKW_DescriptorPool::add_layout(const VKW_DescriptorSetLayout& layout, uint32_t count)
{
	for (const VkDescriptorSetLayoutBinding& binding : layout.get_bindings()) {
		VkDescriptorPoolSize size{};
		size.type = binding.descriptorType;
		size.descriptorCount = binding.descriptorCount * count;

		pool_size.push_back(size);
	}
}
