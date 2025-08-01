#include "Terrain.h"
#include "vk_wrap/VKW_ComputePipeline.h"

#include "DirectionalLight.h"

void SharedTerrainData::init(const VKW_Device* device)
{
	// begin create create descriptor set layout
	// uniform buffer containing projection & view matrix
	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// uniform buffer for rendering depth for shadow map
	descriptor_set_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// terrain texture and sampler
	descriptor_set_layout.add_binding(
		2,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// albedo
	descriptor_set_layout.add_binding(
		3,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// normal map
	descriptor_set_layout.add_binding(
		4,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// curvature map
	descriptor_set_layout.add_binding(
		5,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	// shadow map (seperate sampler and image due to needing two different samplers)
	descriptor_set_layout.add_binding(
		6,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.add_binding(
		7,
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.add_binding(
		8,
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.init(device, "Terrain Desc Layout");
	// end create create descriptor set layout

	push_constant.init(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT , 0);
	vertex_push_constant.init(VK_SHADER_STAGE_VERTEX_BIT, sizeof(TerrainPushConstants));
}

void SharedTerrainData::del()
{
	descriptor_set_layout.del();	
}

void Terrain::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, const VKW_Sampler* sampler, SharedTerrainData* shared_terrain_data, const std::string& height_path, const std::string& albedo_path, const std::string& normal_path, uint32_t mesh_res)
{
	shared_data = shared_terrain_data;
	texture_sampler = sampler;

	// textures to be used
	// todo check if we need graphics pool for create_texture_from_path
	height_map = create_texture_from_path(
		&device,
		&graphics_pool,
		height_path,
		Texture_Type::Tex_R_Linear,
		"Terrain Height map"
	);

	curvatue.init(
		&device,
		height_map.get_width(),
		height_map.get_height(),
		Texture::find_format(device, Texture_Type::Tex_Float),
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(),
		"Terrain Curvature"
	);

	// Compute curvature as a preprocessing step
	precompute_curvature(device, graphics_pool, descriptor_pool);

	// shading
	albedo = create_texture_from_path(
		&device,
		&graphics_pool,
		albedo_path,
		Texture_Type::Tex_RGB,
		"Terrain Albedo"
	);

	normal_map = create_texture_from_path(
		&device,
		&graphics_pool,
		normal_path,
		Texture_Type::Tex_RGB_Linear,
		"Terrain Normals"
	);

	// create cpu mesh
	std::vector<Vertex> terrain_vertices{};
	std::vector<uint32_t> terrain_indices{};

	for (uint32_t iy = 0; iy < mesh_res; iy++) {
		for (uint32_t ix = 0; ix < mesh_res; ix++) {
			float u = ((float)ix) / mesh_res;
			float v = ((float)iy) / mesh_res;
			float pos_x = (u - 0.5) * 2;
			float pos_y = (v - 0.5) * 2;

			// normal is ignored and just computed from the normal map
			terrain_vertices.push_back({ {pos_x,pos_y,0}, u, {0,0,1}, v, {1,0,0,1} });
		}
	}

	for (uint32_t iy = 0; iy < mesh_res - 1; iy++) {
		for (uint32_t ix = 0; ix < mesh_res - 1; ix++) {
			terrain_indices.push_back(ix + iy * mesh_res);
			terrain_indices.push_back(ix + 1 + iy * mesh_res);
			terrain_indices.push_back(ix + 1 + (iy + 1) * mesh_res);
			terrain_indices.push_back(ix + (iy + 1) * mesh_res);
		}
	}

	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.init(&device, &descriptor_pool, shared_data->get_descriptor_set_layout(), "Terrain Desc Set");
	}
	
	mesh.init(device, transfer_pool, terrain_vertices, terrain_indices);
}

void Terrain::set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& general_ubo, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& shadow_map_ubo, Texture& shadow_map, const VKW_Sampler& shadow_map_sampler, const VKW_Sampler& shadow_map_gather_sampler)
{
	// set the bindings
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set = descriptor_sets.at(i);

		set.update(0, general_ubo.at(i));
		set.update(1, shadow_map_ubo.at(i));

		set.update(2, height_map.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler);
		set.update(3, albedo.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler);
		set.update(4, normal_map.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler);
		set.update(5, curvatue.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler);
		
		set.update(6, shadow_map.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, MAX_CASCADE_COUNT));
		set.update(7, shadow_map_sampler);
		set.update(8, shadow_map_gather_sampler);
	}
}

void Terrain::precompute_curvature(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_DescriptorPool& descriptor_pool)
{
	// descriptor sets
	VKW_DescriptorSetLayout compute_layout{};
	// read from height map
	compute_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // read only 
		VK_SHADER_STAGE_COMPUTE_BIT
	);

	// write into curvature texture
	compute_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, // supports load, store and atomic operations
		VK_SHADER_STAGE_COMPUTE_BIT
	);
	compute_layout.init(&device, "Curvature Desc Layout");

	VKW_DescriptorSet compute_desc_set{};
	compute_desc_set.init(&device, &descriptor_pool, compute_layout, "Curvature Desc Set");

	// transition layout of curvatue
	curvatue.transition_layout(&graphics_pool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// update descriptor sets
	compute_desc_set.update(0, height_map.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler);
	compute_desc_set.update(1, curvatue.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), *texture_sampler, VK_IMAGE_LAYOUT_GENERAL);

	// compute pipeline
	VKW_ComputePipeline pipeline{};
	pipeline.add_descriptor_sets({ compute_layout });
	pipeline.init(&device, "shaders/terrain/curvature_comp.spv", "Curvature compute pipeline");

	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		&device,
		&graphics_pool,
		true,
		"Terrain Curvature CMD"
	);

	// execute
	command_buffer.begin_single_use();
	{
		pipeline.bind(command_buffer);
		compute_desc_set.bind(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.get_layout());
		
		pipeline.dispatch(command_buffer, std::ceil(curvatue.get_width() / 16.0), std::ceil(curvatue.get_height() / 16.0), 1);
		
		Texture::transition_layout(command_buffer, curvatue, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	command_buffer.submit_single_use();

	pipeline.del();
	compute_desc_set.del();
	compute_layout.del();
}

void Terrain::del()
{
	height_map.del();
	curvatue.del();
	albedo.del();
	normal_map.del();
	mesh.del();

	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.del();
	}
}