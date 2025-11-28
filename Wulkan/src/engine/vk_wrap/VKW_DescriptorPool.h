#pragma once

#include "VKW_Device.h"
#include "VKW_Object.h"

class VKW_DescriptorPool : public VKW_Object
{
public:
	VKW_DescriptorPool() = default;
	// initializes the pool, before that add descriptors to it
	void init(const VKW_Device* device, uint32_t max_sets, const std::string& obj_name);
	virtual void reset();
	virtual void del() override;

	// returns the pool that was used to allocate the set (this might not match get_descriptor_pool (VKW_DynamicDescriptorPool)
	virtual VkDescriptorPool allocate_descriptor_set(VkDescriptorSet& descr_set, VkDescriptorSetAllocateInfo allocate_info, const std::string name);
protected:
	const VKW_Device* m_device = nullptr;
	std::string m_name;
private:
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> pool_size;
public:
	// expected to be called before init
	virtual void add_type(VkDescriptorType type, uint32_t count);
	// add a layout to the pool size (multiplied by count); expected to be called before init
	virtual void add_layout(const class VKW_DescriptorSetLayout& layout, uint32_t count);

	// should not be called for VKW_DynamicDescriptorPool
	virtual inline VkDescriptorPool get_descriptor_pool() const { return descriptor_pool; };
	// should not be called for VKW_DynamicDescriptorPool
	inline operator VkDescriptorPool() const { return get_descriptor_pool(); } ;
};

inline VkDescriptorPool VKW_DescriptorPool::allocate_descriptor_set(VkDescriptorSet& descr_set, VkDescriptorSetAllocateInfo allocate_info, const std::string name)
{
	allocate_info.descriptorPool = descriptor_pool;
	VK_CHECK_ET(vkAllocateDescriptorSets(*m_device, &allocate_info, &descr_set), RuntimeException, fmt::format("Failed to allocate descriptor set", name));
	return descriptor_pool;
}

class VKW_DynamicDescriptorPool : public VKW_DescriptorPool {
public:
	VKW_DynamicDescriptorPool() = default;
	void init(const VKW_Device* device, uint32_t default_max_sets, const std::string& obj_name);
	void reset() override;
	void del() override;

	VkDescriptorPool allocate_descriptor_set(VkDescriptorSet& descr_set, VkDescriptorSetAllocateInfo allocate_info, const std::string name) override;
private:
	std::vector<VkDescriptorPool> m_current_pools;
	std::vector<VkDescriptorPool> m_full_pools;

	uint32_t m_current_max_sets;
	bool m_stop_growing = false;
	uint32_t m_pool_count;
	std::vector<VkDescriptorPoolSize> m_growing_pool_sizes;
	std::vector<VkDescriptorPoolSize> m_static_pool_sizes;

	VkDescriptorPool get_pool();
	VkDescriptorPool create_pool();
public:
	// per default those can grow
	void add_type(VkDescriptorType type, uint32_t count) override { add_type(type, count, true); };
	// per default those can grow
	void add_layout(const class VKW_DescriptorSetLayout& layout, uint32_t count) override { add_layout(layout, count, true); };

	void add_type(VkDescriptorType type, uint32_t count, bool can_grow);
	void add_layout(const class VKW_DescriptorSetLayout& layout, uint32_t count, bool can_grow);

	// manages multiple descriptor pools, does not leak any to the outside
	VkDescriptorPool get_descriptor_pool() const override {
		throw RuntimeException(
			fmt::format("Called get_descriptor_pool on VKW_DynamicDescriptorPool {}. Not allowed!", m_name),
			__FILE__, __LINE__
		);
	};
};