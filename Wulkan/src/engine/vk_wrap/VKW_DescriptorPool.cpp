#include "common.h"
#include "VKW_DescriptorPool.h"

#include "VKW_DescriptorSet.h"

void VKW_DescriptorPool::init(const VKW_Device* vkw_device, uint32_t max_sets, const std::string& obj_name)
{
	m_device = vkw_device;
	m_name = obj_name;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allows vkFreeDescriptorSets (done by imgui)

	pool_info.pPoolSizes = pool_size.data();
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());

	pool_info.maxSets = max_sets;

	VK_CHECK_ET(vkCreateDescriptorPool(*m_device, &pool_info, VK_NULL_HANDLE, &descriptor_pool), RuntimeException, fmt::format("Failed to create descriptor pool ({})", m_name));
	m_device->name_object((uint64_t) descriptor_pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_name);
}

void VKW_DescriptorPool::reset()
{
	vkResetDescriptorPool(*m_device, descriptor_pool, 0);
}

void VKW_DescriptorPool::del()
{
	VK_DESTROY(descriptor_pool, vkDestroyDescriptorPool, *m_device, descriptor_pool);
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

void VKW_DynamicDescriptorPool::init(const VKW_Device* device, uint32_t default_max_sets, const std::string& obj_name)
{
	m_device = device;
	m_current_max_sets = default_max_sets;
	m_name = obj_name;
}

void VKW_DynamicDescriptorPool::reset()
{
	for (const VkDescriptorPool& pool : m_current_pools) {
		vkResetDescriptorPool(*m_device, pool, 0);
	}

	for (const VkDescriptorPool& pool : m_full_pools) {
		vkResetDescriptorPool(*m_device, pool, 0);
	}
}

void VKW_DynamicDescriptorPool::del()
{
	for (VkDescriptorPool& pool : m_current_pools) {
		VK_DESTROY(pool, vkDestroyDescriptorPool, *m_device, pool);
	}

	for (VkDescriptorPool& pool : m_full_pools) {
		VK_DESTROY(pool, vkDestroyDescriptorPool, *m_device, pool);
	}
}

VkDescriptorPool VKW_DynamicDescriptorPool::allocate_descriptor_set(VkDescriptorSet& descr_set, VkDescriptorSetAllocateInfo allocate_info, const std::string name)
{
	VkDescriptorPool pool = get_pool();

	allocate_info.descriptorPool = pool;
	VkResult res = vkAllocateDescriptorSets(*m_device, &allocate_info, &descr_set);
	if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL) {
		m_full_pools.push_back(pool);

		// reattempt once
		pool = get_pool();

		allocate_info.descriptorPool = pool;
		VK_CHECK_ET(vkAllocateDescriptorSets(*m_device, &allocate_info, &descr_set), RuntimeException, fmt::format("Failed to allocate descriptor set {} on second attempt from dynamic pool {}", name, m_name));
	}
	else if (res) {
		throw RuntimeException(
			fmt::format("Error allocating a descriptor set {} from dynamic pool {}: {}", name, m_name, std::string(string_VkResult(res))),
			__FILE__, __LINE__
		);
	}
	
	m_current_pools.push_back(pool);

	return pool;
}

VkDescriptorPool VKW_DynamicDescriptorPool::get_pool()
{
	if (!m_current_pools.empty()) {
		VkDescriptorPool p = m_current_pools.back();
		m_current_pools.pop_back();
		return p;
	}
	else {
		return create_pool();
	}
}

VkDescriptorPool VKW_DynamicDescriptorPool::create_pool()
{
	VkDescriptorPool pool;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allows vkFreeDescriptorSets (done by imgui)

	std::vector<VkDescriptorPoolSize> pool_sizes{ m_static_pool_sizes };
	pool_sizes.reserve(pool_sizes.size() + m_growing_pool_sizes.size());

	// add to pool_sizes and grow pool sizes for next creation
	for (VkDescriptorPoolSize& size : m_growing_pool_sizes) {
		pool_sizes.push_back(size);
		if (!m_stop_growing)
			size.descriptorCount = static_cast<uint32_t> (size.descriptorCount * 1.5);
	}

	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());

	pool_info.maxSets = m_current_max_sets;
	if (!m_stop_growing) {
		m_current_max_sets = static_cast<uint32_t> (m_current_max_sets * 1.5); // some hard limit
		if (m_current_max_sets > 2048) {
			m_stop_growing = true;
			m_current_max_sets = 2048;
		}
	}

	std::string name = fmt::format("{} ({})", m_name, m_pool_count++);
	VK_CHECK_ET(vkCreateDescriptorPool(*m_device, &pool_info, VK_NULL_HANDLE, &pool), RuntimeException, fmt::format("Failed to create descriptor pool ({})", name));
	m_device->name_object((uint64_t)pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);

	return pool;
}

void VKW_DynamicDescriptorPool::add_type(VkDescriptorType type, uint32_t count, bool can_grow)
{
	VkDescriptorPoolSize size{};
	size.type = type;
	size.descriptorCount = count;

	if (can_grow) {
		m_growing_pool_sizes.push_back(size);
	}
	else {
		m_static_pool_sizes.push_back(size);
	}
}

void VKW_DynamicDescriptorPool::add_layout(const VKW_DescriptorSetLayout& layout, uint32_t count, bool can_grow)
{
	for (const VkDescriptorSetLayoutBinding& binding : layout.get_bindings()) {
		VkDescriptorPoolSize size{};
		size.type = binding.descriptorType;
		size.descriptorCount = binding.descriptorCount * count;

		if (can_grow) {
			m_growing_pool_sizes.push_back(size);
		}
		else {
			m_static_pool_sizes.push_back(size);
		}
	}
}
