#include "VKW_DescriptorSet.h"

void VKW_DescriptorSetLayout::init(const VKW_Device* vkw_device)
{
	device = vkw_device;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pBindings = bindings.data();
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	VK_CHECK_ET(vkCreateDescriptorSetLayout(*device, &layout_info, VK_NULL_HANDLE, &layout), RuntimeException, "Failed to create descriptor set layout");
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

void VKW_DescriptorSet::init(const VKW_Device* vkw_device, const VKW_DescriptorPool* vkw_pool, VKW_DescriptorSetLayout layout)
{
	device = vkw_device;
	pool = vkw_pool;

	VkDescriptorSetAllocateInfo set_info{};
	set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_info.descriptorPool = *pool;

	VkDescriptorSetLayout desc_layout = layout;
	set_info.pSetLayouts = &desc_layout;

	set_info.descriptorSetCount = 1;

	VK_CHECK_ET(vkAllocateDescriptorSets(*device, &set_info, &descriptor_set), RuntimeException, "Failed to allocate descriptor set");


	for (const VkDescriptorSetLayoutBinding& binding : layout.get_bindings()) {
		binding_types[binding.binding] = binding.descriptorType;
	}
}

void VKW_DescriptorSet::bind(const VKW_CommandBuffer& command_buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout) const
{
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

void VKW_DescriptorSet::update(uint32_t binding, Texture& texture, const VKW_Sampler& sampler, VkImageLayout layout, VkImageAspectFlags aspect) const
{
	VkDescriptorImageInfo image_info{};
	image_info.imageLayout = layout;
	image_info.imageView = texture.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT);
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
