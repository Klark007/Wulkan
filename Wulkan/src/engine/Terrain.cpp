#include "Terrain.h"

void SharedTerrainData::init(const VKW_Device* device)
{
	// begin create create descriptor set layout
	// uniform buffer containing projection & view matrix
	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
	);

	// terrain texture and sampler
	descriptor_set_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.init(device);
	// end create create descriptor set layout

	push_constant.init(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT , 0);
	vertex_push_constant.init(VK_SHADER_STAGE_VERTEX_BIT, sizeof(TerrainPushConstants));
}

void SharedTerrainData::del()
{
	descriptor_set_layout.del();	
}

void Terrain::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool* descriptor_pool, SharedTerrainData* shared_terrain_data, const std::string& path, uint32_t mesh_res)
{
	shared_data = shared_terrain_data;

	// todo check if we need graphics pool for create_texture_from_path
	height_map = create_texture_from_path(
		&device,
		&graphics_pool,
		path,
		Texture_Type::Tex_R
	);

	std::vector<Vertex> terrain_vertices{};
	std::vector<uint32_t> terrain_indices{};

	for (uint32_t iy = 0; iy < mesh_res; iy++) {
		for (uint32_t ix = 0; ix < mesh_res; ix++) {
			float u = ((float)ix) / mesh_res;
			float v = ((float)iy) / mesh_res;
			float pos_x = (u - 0.5) * 2;
			float pos_y = (v - 0.5) * 2;

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
		set.init(&device, descriptor_pool, shared_data->get_descriptor_set_layout());
	}
	
	mesh.init(device, transfer_pool, terrain_vertices, terrain_indices);
}

void Terrain::set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers, const VKW_Sampler& texture_sampler)
{
	// set the bindings
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set = descriptor_sets.at(i);

		set.update(0, uniform_buffers.at(i));
		set.update(1, height_map, texture_sampler, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Terrain::del()
{
	height_map.del();
	mesh.del();

	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.del();
	}
}