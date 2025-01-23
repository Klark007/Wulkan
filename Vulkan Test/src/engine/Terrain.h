#pragma once

#include "vk_types.h"
#include "Shape.h"
#include "Mesh.h"
#include "Texture.h"

// TODO: Per Shape model transform
#include <glm/gtc/matrix_transform.hpp>


#include "vk_wrap/VKW_GraphicsPipeline.h"

struct TerrainVertexPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
};

struct TerrainPushConstants {
	alignas(16) glm::mat4 model;

	alignas(4) float tesselation_strength;
	alignas(4) float height_scale;
};

// singleton shared between all Terrain instances
class SharedTerrainData : public VKW_Object {
public:
	SharedTerrainData() = default;
	// initializes data
	void init(const VKW_Device* device);
	void del() override;
private:
	VKW_DescriptorSetLayout descriptor_set_layout;

	// TODO: are there potential race conditions with the push constants being shared between multiple Terrain's
	VKW_PushConstants<TerrainPushConstants> push_constants;
	VKW_PushConstants<TerrainVertexPushConstants> vertex_push_constants;
public:
	const VKW_DescriptorSetLayout& get_descriptor_set_layout() const { return descriptor_set_layout; };
	
	inline std::vector<VkPushConstantRange> get_push_consts_range() const {
		return { vertex_push_constants.get_range(), push_constants.get_range() };
	}

	inline VKW_PushConstants<TerrainVertexPushConstants>& get_vertex_pc() { return vertex_push_constants; };
	inline VKW_PushConstants<TerrainPushConstants>& get_pc() { return push_constants; };
};

class Terrain : public Shape
{
public:
	Terrain() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool* descriptor_pool, SharedTerrainData* shared_terrain_data, const std::string& path, uint32_t mesh_res);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers, const VKW_Sampler& texture_sampler);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
public: // TODO remove
	Mesh mesh;
	Texture height_map;

private: // TODO remove
	float tesselation_strength;
	float height_scale;

	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
	SharedTerrainData* shared_data;
public:
	static VKW_GraphicsPipeline create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedTerrainData& shared_terrain_data);
	
	inline void set_tesselation_strength(float strength) { tesselation_strength = strength; };
	inline void set_height_scale(float scale) { height_scale = scale; };
};

inline void Terrain::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
	descriptor_sets.at(current_frame).bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout());

	shared_data->get_pc().update({
		glm::scale(glm::mat4(1), glm::vec3(25.0,25.0,1.0)),
		tesselation_strength,
		height_scale * 25
	});
	shared_data->get_pc().push(command_buffer, pipeline.get_layout());

	shared_data->get_vertex_pc().update({
		mesh.get_vertex_address()
	});
	shared_data->get_vertex_pc().push(command_buffer, pipeline.get_layout());

	mesh.draw(command_buffer, current_frame, pipeline);
}

inline VKW_GraphicsPipeline Terrain::create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedTerrainData& shared_terrain_data)
{
	VKW_Shader terrain_vert_shader{};
	terrain_vert_shader.init(device, "shaders/terrain_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	VKW_Shader terrain_frag_shader{};
	terrain_frag_shader.init(device, "shaders/terrain_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VKW_Shader tess_ctrl_shader{};
	tess_ctrl_shader.init(device, "shaders/terrain_tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

	VKW_Shader tess_eval_shader{};
	tess_eval_shader.init(device, "shaders/terrain_tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

	VKW_GraphicsPipeline graphics_pipeline{};
	
	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	graphics_pipeline.set_culling_mode(VK_CULL_MODE_NONE); // TODO backface culling
	//graphics_pipeline.set_wireframe_mode();
	graphics_pipeline.enable_depth_test();
	graphics_pipeline.enable_depth_write();

	graphics_pipeline.enable_tesselation();
	graphics_pipeline.set_tesselation_patch_size(4);

	graphics_pipeline.add_shader_stages({ terrain_vert_shader, terrain_frag_shader, tess_ctrl_shader, tess_eval_shader });
	graphics_pipeline.add_descriptor_sets({ shared_terrain_data.get_descriptor_set_layout() });

	graphics_pipeline.add_push_constants(shared_terrain_data.get_push_consts_range());

	graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	graphics_pipeline.set_color_attachment(
		color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
		true,
		{ {0.0f, 0.0f, 0.0f, 1.0f} }
	);

	graphics_pipeline.set_depth_attachment(
		depth_rt.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
		true,
		1.0f
	);

	graphics_pipeline.init(device);

	terrain_vert_shader.del();
	terrain_frag_shader.del();
	tess_ctrl_shader.del();
	tess_eval_shader.del();

	return graphics_pipeline;
}
