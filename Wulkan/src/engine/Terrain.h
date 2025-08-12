#pragma once

#include "common.h"
#include "Shape.h"
#include "Mesh.h"
#include "Texture.h"

#include "vk_wrap/VKW_GraphicsPipeline.h"

struct TerrainVertexPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
};

enum TerrainVisualizationMode {
	Shaded,
	Level,
	Height,
	Normal,
	Error,
	ShadowDepth,
	ShadowCascade,
};

struct TerrainPushConstants {
	alignas(16) glm::mat4 model;

	alignas(4) float tesselation_strength;
	alignas(4) float max_tesselation;
	alignas(4) float texture_eps;

	alignas(4) TerrainVisualizationMode visualization_mode;
	alignas(4) int cascade_idx;
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
	VKW_PushConstant<TerrainPushConstants> push_constant;
	VKW_PushConstant<TerrainVertexPushConstants> vertex_push_constant;
public:
	const VKW_DescriptorSetLayout& get_descriptor_set_layout() const { return descriptor_set_layout; };
	
	inline std::vector<VkPushConstantRange> get_push_consts_range() const {
		return { vertex_push_constant.get_range(), push_constant.get_range()};
	}

	inline VKW_PushConstant<TerrainVertexPushConstants>& get_vertex_pc() { return vertex_push_constant; };
	inline VKW_PushConstant<TerrainPushConstants>& get_pc() { return push_constant; };
};

class Terrain : public Shape
{
public:
	Terrain() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, const VKW_Sampler* sampler, SharedTerrainData* shared_terrain_data, const std::string& height_path, const std::string& albedo_path, const std::string& normal_path, uint32_t mesh_res);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& general_ubo, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& shadow_map_ubo, Texture& shadow_map, const VKW_Sampler& shadow_map_sampler, const VKW_Sampler& shadow_map_gather_sampler);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
private:
	Mesh mesh;
	const VKW_Sampler* texture_sampler;
	Texture height_map;
	Texture albedo;
	Texture normal_map;

	Texture curvatue;
	// precomputes the curvature from the height map using a compute shader
	void precompute_curvature(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_DescriptorPool& descriptor_pool);

	float tesselation_strength;
	float max_tesselation;
	float texture_eps;

	TerrainVisualizationMode visualization_mode;

	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
	SharedTerrainData* shared_data;
public:
	static VKW_GraphicsPipeline create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedTerrainData& shared_terrain_data, bool depth_only = false, bool wireframe_mode = false, bool bias_depth = false, int nr_shadow_cascades = 3);
	
	inline void set_tesselation_strength(float strength) { tesselation_strength = strength; }; // multiplicative factor of the tesselation level computed
	inline void set_max_tesselation(float max) { max_tesselation = max; }; // maximum tesselation level
	inline void set_texture_eps(float eps) { texture_eps = eps; }; // used for delta u,v for computing texture derivatives and curvature
	inline void set_visualization_mode(TerrainVisualizationMode mode) { visualization_mode = mode; }; // switch between shading and debug views
};

inline void Terrain::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
	descriptor_sets.at(current_frame).bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout());

	shared_data->get_pc().update({
		model,
		tesselation_strength,
		max_tesselation,
		texture_eps,
		visualization_mode,
		cascade_idx
	});
	shared_data->get_pc().push(command_buffer, pipeline.get_layout());

	shared_data->get_vertex_pc().update({
		mesh.get_vertex_address()
	});
	shared_data->get_vertex_pc().push(command_buffer, pipeline.get_layout());

	mesh.draw(command_buffer, current_frame, pipeline);
}

inline VKW_GraphicsPipeline Terrain::create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedTerrainData& shared_terrain_data, bool depth_only, bool wireframe_mode, bool bias_depth, int nr_shadow_cascades)
{
	VKW_Shader terrain_vert_shader{};
	terrain_vert_shader.init(device, "shaders/terrain/terrain_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "Terrain vertex shader");

	VKW_SpecializationConstants<int, 1> specialization_const{};
	specialization_const.init({ nr_shadow_cascades }, { 0 });

	VKW_Shader terrain_frag_shader{};
	terrain_frag_shader.init(device, "shaders/terrain/terrain_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "Terrain fragment shader", "main", specialization_const.get_info());

	VKW_Shader tess_ctrl_shader{};
	if (!depth_only) {
		tess_ctrl_shader.init(device, "shaders/terrain/terrain_tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "Terrain tesselation control shader");
	}
	else {
		tess_ctrl_shader.init(device, "shaders/terrain/terrain_depth_tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "Terrain-depth tesselation control shader", "main", specialization_const.get_info());
	}
	VKW_Shader tess_eval_shader{};
	if (!depth_only) {
		tess_eval_shader.init(device, "shaders/terrain/terrain_tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "Terrain tesselation evaluation shader");
	}
	else {
		tess_eval_shader.init(device, "shaders/terrain/terrain_depth_tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "Terrain-depth tesselation evaluation shader", "main", specialization_const.get_info());
	}

	VKW_GraphicsPipeline graphics_pipeline{};
	
	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	if (wireframe_mode) {
		graphics_pipeline.set_wireframe_mode();
	}
	graphics_pipeline.set_culling_mode();
	graphics_pipeline.enable_depth_test();
	graphics_pipeline.enable_depth_write();

	graphics_pipeline.enable_tesselation();
	graphics_pipeline.set_tesselation_patch_size(4);

	if (!depth_only) {
		graphics_pipeline.add_shader_stages({ terrain_vert_shader, terrain_frag_shader, tess_ctrl_shader, tess_eval_shader });
	}
	else {
		graphics_pipeline.add_shader_stages({ terrain_vert_shader, tess_ctrl_shader, tess_eval_shader });
	}
	graphics_pipeline.add_descriptor_sets({ shared_terrain_data.get_descriptor_set_layout() });

	graphics_pipeline.add_push_constants(shared_terrain_data.get_push_consts_range());

	if (!depth_only) {
		graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	}
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());
	
	if (depth_only && bias_depth) {
		graphics_pipeline.enable_dynamic_depth_bias();
	}

	graphics_pipeline.init(device, "Terrain graphics pipeline");

	terrain_vert_shader.del();
	terrain_frag_shader.del();
	tess_ctrl_shader.del();
	tess_eval_shader.del();

	return graphics_pipeline;
}
