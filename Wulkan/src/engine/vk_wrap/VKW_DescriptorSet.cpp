#include "VKW_DescriptorSet.h"
#include <format>

void VKW_DescriptorSetLayout::init(const VKW_Device* vkw_device, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pBindings = bindings.data();
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	
	VK_CHECK_ET(vkCreateDescriptorSetLayout(*device, &layout_info, VK_NULL_HANDLE, &layout), RuntimeException, std::format("Failed to create descriptor set layout ({})", name));
	device->name_object((uint64_t)layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
}

void VKW_DescriptorSetLayout::del()
{
	VK_DESTROY(layout, vkDestroyDescriptorSetLayout, *device, layout);
}

void VKW_DescriptorSetLayout::add_binding(uint32_t binding_slot, VkDescriptorType type, VkShaderStageFlags shader_stages)
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = binding_slot;
	binding.descriptorCount = 1; // length for arrays
	binding.descriptorType = type;
	binding.stageFlags = shader_stages;

	bindings.push_back(binding);
}

void VKW_DescriptorSet::init(const VKW_Device* vkw_device, const VKW_DescriptorPool* vkw_pool, VKW_DescriptorSetLayout layout, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;
	pool = vkw_pool;

	VkDescriptorSetAllocateInfo set_info{};
	set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_info.descriptorPool = *pool;

	VkDescriptorSetLayout desc_layout = layout;
	set_info.pSetLayouts = &desc_layout;

	set_info.descriptorSetCount = 1;

	VK_CHECK_ET(vkAllocateDescriptorSets(*device, &set_info, &descriptor_set), RuntimeException, std::format("Failed to allocate descriptor set", name));


	for (const VkDescriptorSetLayoutBinding& binding : layout.get_bindings()) {
		binding_types[binding.binding] = binding.descriptorType;
	}
}

void VKW_DescriptorSet::bind(const VKW_CommandBuffer& command_buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout) const
{
	// TODO: Change first set (0) to bind multiple sets at the same time
	vkCmdBindDescriptorSets(command_buffer, bind_point, layout, 0, 1, &descriptor_set, 0, VK_NULL_HANDLE);
}

void VKW_DescriptorSet::del()
{
	VK_DESTROY_FROM(descriptor_set, vkFreeDescriptorSets, *device, *pool, 1, &descriptor_set);
}

void VKW_DescriptorSet::update(uint32_t binding, const VKW_Buffer& buffer) const
{
	VkDescriptorBufferInfo buffer_info{};
	buffer_info.buffer = buffer;
	buffer_info.offset = 0;
	buffer_info.range  = buffer.size();

	VkWriteDescriptorSet buffer_write{};
	buffer_write.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	buffer_write.dstSet = descriptor_set;
	buffer_write.dstBinding = binding;
	buffer_write.descriptorType = binding_types.at(binding);

	buffer_write.dstArrayElement = 0;
	buffer_write.descriptorCount = 1;

	buffer_write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(*device, 1, &buffer_write, 0, VK_NULL_HANDLE);
}

void VKW_DescriptorSet::update(uint32_t binding, VkImageView image_view, const VKW_Sampler& sampler, VkImageLayout layout) const
{
	VkDescriptorImageInfo image_info{};
	image_info.imageLayout = layout;
	image_info.imageView = image_view;
	image_info.sampler = sampler;

	VkWriteDescriptorSet image_write{};
	image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	image_write.dstSet = descriptor_set;
	image_write.dstBinding = binding;
	image_write.descriptorType = binding_types.at(binding);

	image_write.dstArrayElement = 0;
	image_write.descriptorCount = 1;
	image_write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(*device, 1, &image_write, 0, VK_NULL_HANDLE);
}

void VKW_DescriptorSet::update(uint32_t binding, VkImageView image_view, VkImageLayout layout) const
{
	VkDescriptorImageInfo image_info{};
	image_info.imageLayout = layout;
	image_info.imageView = image_view;

	VkWriteDescriptorSet image_write{};
	image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	image_write.dstSet = descriptor_set;
	image_write.dstBinding = binding;
	image_write.descriptorType = binding_types.at(binding);

	image_write.dstArrayElement = 0;
	image_write.descriptorCount = 1;
	image_write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(*device, 1, &image_write, 0, VK_NULL_HANDLE);
}

void VKW_DescriptorSet::update(uint32_t binding, const VKW_Sampler& sampler) const
{
	VkDescriptorImageInfo image_info{};
	image_info.sampler = sampler;

	VkWriteDescriptorSet sampler_write{};
	sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sampler_write.dstSet = descriptor_set;
	sampler_write.dstBinding = binding;
	sampler_write.descriptorType = binding_types.at(binding);

	sampler_write.dstArrayElement = 0;
	sampler_write.descriptorCount = 1;
	sampler_write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(*device, 1, &sampler_write, 0, VK_NULL_HANDLE);
}
