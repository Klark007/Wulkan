#include "EnvironmentMap.h"

void EnvironmentMap::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<EnvironmentMapPushConstants, 2>& render_pass, const std::string& path)
{
	material.init(device, descriptor_pool, render_pass, "Environment map Material");

	cube_map = create_cube_map_from_path(&device, &graphics_pool, path, Tex_HDR_RGBA, "Environment map");

	// does not have correct normals or uv's as those are not used in the environment map shader
	std::vector<Vertex> cube_vertices = {
		{{-1,-1,-1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{1,-1,-1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{-1,1,-1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{1,1,-1}, 0, {0,0,1}, 0, {1,0,1,1}},

		{{-1,-1,1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{1,-1,1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{-1,1,1}, 0, {0,0,1}, 0, {1,0,1,1}},
		{{1,1,1}, 0, {0,0,1}, 0, {1,0,1,1}}
	};
	std::vector<uint32_t> cube_indices = {
		// bottom
		0,1,2,
		1,3,2,
		// left
		0,5,1,
		0,4,5,
		// right
		2,3,6,
		3,7,6,
		// front
		0,2,6,
		0,6,4,
		// back
		1,5,3,
		3,5,7,
		// top
		4,7,5,
		4,6,7

	};

	mesh.init(device, transfer_pool, cube_vertices, cube_indices);
}

void EnvironmentMap::set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers, const VKW_Sampler& texture_sampler)
{
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set0 = material.get_descriptor_set(i, 0);
		const VKW_DescriptorSet& set1 = material.get_descriptor_set(i, 1);

		set0.update(0, uniform_buffers.at(i));

		// need all 6 faces of the array/cubemap
		set1.update(0, cube_map.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 0, 6), texture_sampler);
	}
}

RenderPass<EnvironmentMapPushConstants, 2> EnvironmentMap::create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 2>& layouts, Texture& color_rt, Texture& depth_rt)
{
	RenderPass<EnvironmentMapPushConstants, 2> render_pass{};

	// Push constants
	VKW_PushConstant<EnvironmentMapPushConstants> push_constant{};
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT , 0);

	// Graphics pipeline
	VKW_GraphicsPipeline graphics_pipeline{};

	VKW_Shader vert_shader{};
	vert_shader.init(device, "shaders/environment_maps/environment_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "Environment map vertex shader");

	VKW_Shader frag_shader{};
	frag_shader.init(device, "shaders/environment_maps/environment_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "Environment map fragment shader");

	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	graphics_pipeline.set_culling_mode();

	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });

	graphics_pipeline.add_descriptor_sets(layouts);

	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	graphics_pipeline.init(device, "Environment graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	render_pass.init(
		graphics_pipeline,
		layouts,
		push_constant,
		"Environment Renderpass"
	);

	return render_pass;
}

VKW_DescriptorSetLayout EnvironmentMap::create_descriptor_set_layout(const VKW_Device& device)
{
	VKW_DescriptorSetLayout descriptor_set_layout{};

	// cube map
	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);
	descriptor_set_layout.init(&device, "Environment Desc Layout");

	return descriptor_set_layout;
}

void EnvironmentMap::del()
{
	cube_map.del();
	mesh.del();
	material.del();
}
