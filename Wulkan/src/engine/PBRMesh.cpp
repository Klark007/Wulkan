#include "common.h"
#include "PBRMesh.h"

void PBRMesh::set_descriptor_bindings(Texture& texture_fallback, const VKW_Sampler& general_sampler)
{
	for (size_t mat_idx = 0; mat_idx < m_materials.size(); mat_idx++) {
		auto& mat = m_materials[mat_idx];
		for (unsigned int frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; frame_idx++) {
			const VKW_DescriptorSet& set = mat.get_descriptor_set(frame_idx, 0);

			set.update(0, m_materials[mat_idx].get_uniform_buffer(frame_idx));
			set.update(1, m_materials[mat_idx].get_diffuse_texture(texture_fallback).get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), general_sampler);
		}
	}
}

RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> PBRMesh::create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, PBR_MAT_DESC_SET_COUNT>& layouts, Texture& color_rt, Texture& depth_rt, VkSampleCountFlagBits sample_count, bool depth_only, bool bias_depth, bool cull_backfaces)
{
	RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> render_pass{};

	// Push constants
	VKW_PushConstant<PushConstants> push_constant{};
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT);

	// Graphics pipeline
	VKW_GraphicsPipeline graphics_pipeline{};

	VKW_Shader vert_shader{};
	if (!depth_only) {
		vert_shader.init(device, "shaders/pbr/pbr_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "PBR vertex shader");
	}
	else {
		vert_shader.init(device, "shaders/pbr/pbr_depth_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "PBR vertex shader");
	}

	VKW_Shader frag_shader{};
	if (!depth_only) {
		frag_shader.init(device, "shaders/pbr/pbr_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "PBR fragment shader");
	}
	else {
		frag_shader.init(device, "shaders/pbr/pbr_depth_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "PBR fragment shader");
	}
	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	if (cull_backfaces) {
		graphics_pipeline.set_culling_mode();
	}
	graphics_pipeline.enable_depth_test();
	graphics_pipeline.enable_depth_write();

	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });

	graphics_pipeline.add_descriptor_sets(layouts);
	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	if (!depth_only) {
		graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	}
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	if (depth_only && bias_depth) {
		graphics_pipeline.enable_dynamic_depth_bias();
	}
	
	graphics_pipeline.set_sample_count(sample_count);

	std::string pipeline_name;
	if (depth_only) {
		pipeline_name = "PBR DEPTH graphics pipeline";
	}
	else {
		pipeline_name = "PBR graphics pipeline";
	}
	graphics_pipeline.init(device, pipeline_name);

	vert_shader.del();
	frag_shader.del();

	// end graphics pipeline

	render_pass.init(
		graphics_pipeline,
		layouts,
		push_constant
	);

	return render_pass;
}

VKW_DescriptorSetLayout PBRMesh::create_descriptor_set_layout(const VKW_Device& device)
{
	descriptor_set_layout = VKW_DescriptorSetLayout{};

	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// diffuse texture
	descriptor_set_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.init(&device, "PBR Descriptor Set Layout");

	return descriptor_set_layout;
}
