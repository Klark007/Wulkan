#include "VKW_GraphicsPipeline.h"

VKW_GraphicsPipeline::VKW_GraphicsPipeline()
	: shader_stages{}, descriptor_set_layouts{}
{
	clear();
}

void VKW_GraphicsPipeline::init(const VKW_Device* vkw_device, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;

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
	if (dynamic_depth_bias) {
		dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	}

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();
	pipeline_info.pDynamicState = &dynamic_state;

	// pipeline layout
	pipeline_layout_info.pPushConstantRanges = push_consts_range.data();
	pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_consts_range.size());

	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());

	VK_CHECK_ET(vkCreatePipelineLayout(*device, &pipeline_layout_info, nullptr, &layout), RuntimeException, std::format("Failed to create graphics pipeline layout ({})", name + " layout"));
	pipeline_info.layout = layout;

	VK_CHECK_ET(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipeline_info, VK_NULL_HANDLE, &graphics_pipeline), RuntimeException, std::format("Failed to create graphics pipeline ({})", name));
	device->name_object((uint64_t)graphics_pipeline, VK_OBJECT_TYPE_PIPELINE, name);
	device->name_object((uint64_t)layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name +" layout");
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


	color_attachment_format = VK_FORMAT_UNDEFINED;
	depth_attachment_format = VK_FORMAT_UNDEFINED;


	attachment_state = {};
	attachment_state.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	attachment_state.layerCount = 1;
	attachment_state.renderArea.offset = { 0, 0 };


	render_extent = {};

	viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor = {};
	scissor.offset = { 0, 0 };

	color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

	depth_attachment_info = {};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
}

void VKW_GraphicsPipeline::add_shader_stages(const std::vector<VKW_Shader>& stages)
{
	for (const VKW_Shader& shader : stages) {
		shader_stages.push_back(shader.get_info());
	}
}

void VKW_GraphicsPipeline::add_descriptor_sets(const std::vector<VKW_DescriptorSetLayout>& layouts)
{
	for (const VKW_DescriptorSetLayout layout : layouts) {
		descriptor_set_layouts.push_back(layout);
	}
}

void VKW_GraphicsPipeline::add_push_constants(const std::vector<VkPushConstantRange>& ranges)
{
	push_consts_range.insert(std::end(push_consts_range), std::begin(ranges), std::end(ranges));
}

void VKW_GraphicsPipeline::set_color_attachment(VkImageView attachment, bool do_clear_color, VkClearColorValue clear_color_value)
{
	
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	if (do_clear_color) {
		color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment_info.clearValue.color = clear_color_value;
	}
	else {
		color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	
	color_attachment_info.imageView = attachment;
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachment_state.pColorAttachments = &color_attachment_info;
	attachment_state.colorAttachmentCount = 1;
}

void VKW_GraphicsPipeline::set_depth_attachment(VkImageView attachment, bool do_clear_depth, float clear_depth_value)
{
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	if (do_clear_depth) {
		depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment_info.clearValue.depthStencil.depth = clear_depth_value;
	}
	else {
		depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}

	depth_attachment_info.imageView = attachment;
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachment_state.pDepthAttachment = &depth_attachment_info;
}

void VKW_GraphicsPipeline::set_render_size(VkExtent2D extend)
{
	render_extent = extend;

	attachment_state.renderArea.extent = render_extent;

	viewport.width = static_cast<float>(render_extent.width);
	viewport.height = static_cast<float>(render_extent.height);

	scissor.extent = render_extent;
}