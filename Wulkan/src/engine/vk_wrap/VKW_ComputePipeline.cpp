#include "VKW_ComputePipeline.h"

void VKW_ComputePipeline::init(const VKW_Device* vkw_device, const std::string& shader_path)
{
	device = vkw_device;

	VkComputePipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	

	// Compute shader associated with pipeline
	VKW_Shader compute_shader{};
	compute_shader.init(device, shader_path, VK_SHADER_STAGE_COMPUTE_BIT);
	pipeline_info.stage = compute_shader.get_info();

	// Pipeline layout
	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	layout_info.pPushConstantRanges = push_consts_range.data();
	layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_consts_range.size());

	layout_info.pSetLayouts = descriptor_set_layouts.data();
	layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());

	VK_CHECK_ET(vkCreatePipelineLayout(*device, &layout_info, nullptr, &layout), RuntimeException, "Failed to create compute pipeline layout");
	pipeline_info.layout = layout;
	

	VK_CHECK_ET(
		vkCreateComputePipelines(*device, VK_NULL_HANDLE, 1, &pipeline_info, VK_NULL_HANDLE, &compute_pipeline),
		RuntimeException,
		"Failed to create compute pipeline"
	);

	compute_shader.del();
}

void VKW_ComputePipeline::del()
{
	VK_DESTROY(layout, vkDestroyPipelineLayout, *device, layout);
	VK_DESTROY(compute_pipeline, vkDestroyPipeline, *device, compute_pipeline);
}

void VKW_ComputePipeline::add_descriptor_sets(const std::vector<VKW_DescriptorSetLayout>& layouts)
{
	descriptor_set_layouts.insert(std::end(descriptor_set_layouts), std::begin(layouts), std::end(layouts));
}

void VKW_ComputePipeline::add_push_constants(const std::vector<VkPushConstantRange>& ranges)
{
	push_consts_range.insert(std::end(push_consts_range), std::begin(ranges), std::end(ranges));
}
