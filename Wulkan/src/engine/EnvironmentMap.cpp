#include "EnvironmentMap.h"

void SharedEnvironmentData::init(const VKW_Device* device)
{
	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);

	// cube map
	descriptor_set_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);
	descriptor_set_layout.init(device, "Environment Desc Layout");

	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT);
}

void SharedEnvironmentData::del()
{
	descriptor_set_layout.del();
}

void EnvironmentMap::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedEnvironmentData* shared_environment_data, const std::string& path)
{
	shared_data = shared_environment_data;

	cube_map = create_cube_map_from_path(&device, &graphics_pool, path, Tex_HDR_RGBA, "Environment map");

	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.init(&device, &descriptor_pool, shared_data->get_descriptor_set_layout(), "Environment Desc Set");
	}

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
		const VKW_DescriptorSet& set = descriptor_sets.at(i);

		set.update(0, uniform_buffers.at(i));
		// need all 6 faces of the array/cubemap
		set.update(1, cube_map.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 0, 6), texture_sampler);
	}
}

void EnvironmentMap::del()
{
	cube_map.del();
	mesh.del();

	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.del();
	}
}
