#include "VKW_GraphicsPipeline.h"

VKW_GraphicsPipeline::VKW_GraphicsPipeline()
	: shader_stages{}, descriptor_set_layouts{}
{
	clear();
}

void VKW_GraphicsPipeline::init(const VKW_Device* vkw_device)
{
	device = vkw_device;

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipeline_info.pNext = &render_info;

	pipeline_info.pStages = shader_stages.data();
	pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());

	pipeline_info.pInputAssemblyState = &input_assembly;

	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pDepthStencilState = &depth_stencil;

	if (tesselation_enabled) {
		pipeline_info.pTessellationState = &tesselation;
	}

	pipeline_info.pMultisampleState = &multisampling;

	// color blending
	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blending_attachement;

	pipeline_info.pColorBlendState = &color_blending;
	
	// use vertex pulling
	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_info.pVertexInputState = &vertex_input;

	// use dynamic viewport
	VkPipelineViewportStateCreateInfo viewport{};
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	pipeline_info.pViewportState = &viewport;

	std::vector<VkDynamicState> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();
	pipeline_info.pDynamicState = &dynamic_state;

	// pipeline layout
	VK_CHECK_ET(vkCreatePipelineLayout(*device, &pipeline_layout_info, nullptr, &layout), RuntimeException, "Failed to create graphics pipeline layout");
	pipeline_info.layout = layout;

	VK_CHECK_ET(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipeline_info, VK_NULL_HANDLE, &graphics_pipeline), RuntimeException, "Failed to create graphics pipeline");
}

void VKW_GraphicsPipeline::del()
{
	VK_DESTROY(layout, vkDestroyPipelineLayout, *device, layout);
	VK_DESTROY(graphics_pipeline, vkDestroyPipeline, *device, graphics_pipeline);
}

void VKW_GraphicsPipeline::clear()
{
	shader_stages.clear();

	input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; // Otherwise clamps polygon to near and farpline instead of discarding
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // Otherwise no geometry passes through frame buffer
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.depthBiasEnable = VK_FALSE;

	depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_FALSE;
	depth_stencil.depthWriteEnable = VK_FALSE;

	tesselation_enabled = false;
	tesselation = {};
	tesselation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

	multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;

	color_blending_attachement = {};
	color_blending_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	color_blending_attachement.blendEnable = VK_FALSE; // dont blend
	
	render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

	pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
}

void VKW_GraphicsPipeline::add_shader_stages(const std::vector<VKW_Shader>& stages)
{
	for (const VKW_Shader& shader : stages) {
		shader_stages.push_back(shader.get_info());
	}
}

void VKW_GraphicsPipeline::add_descriptor_sets(const std::vector<VKW_DescriptorSetLayout> layouts)
{
	for (const VKW_DescriptorSetLayout layout : layouts) {
		descriptor_set_layouts.push_back(layout);
	}

	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
}
