#pragma once

#include "common.h"
#include "Path.h"
#include "Shape.h"
#include "Mesh.h"
#include "Texture.h"
#include "Renderpass.h"

#include "vk_wrap/VKW_GraphicsPipeline.h"

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
	alignas(8) VkDeviceAddress vertex_buffer;

	alignas(4) float tesselation_strength;
	alignas(4) float max_tesselation;
	alignas(4) float texture_eps;

	alignas(4) TerrainVisualizationMode visualization_mode;
	alignas(4) int cascade_idx;
};

class Terrain : public Shape
{
public:
	Terrain() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, const VKW_Sampler* sampler, RenderPass<TerrainPushConstants, 3>& render_pass, const VKW_Path& height_path, const VKW_Path& albedo_path, const VKW_Path& normal_path, uint32_t mesh_res);
	void set_descriptor_bindings();
	void del() override;

	// creates singleton render pass, needs to be deleted by caller of function
	// depth_bias only works in depth_only mode
	static RenderPass<TerrainPushConstants, 3> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 3>& layouts, Texture& color_rt, Texture& depth_rt, bool depth_only = false, bool wireframe_mode = false, bool bias_depth = false, int nr_shadow_cascades = 3);
	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
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

	inline static VKW_DescriptorSetLayout descriptor_set_layout;
	MaterialInstance<TerrainPushConstants, 1> material;
public:
	inline void set_tesselation_strength(float strength) { tesselation_strength = strength; }; // multiplicative factor of the tesselation level computed
	inline void set_max_tesselation(float max) { max_tesselation = max; }; // maximum tesselation level
	inline void set_texture_eps(float eps) { texture_eps = eps; }; // used for delta u,v for computing texture derivatives and curvature
	inline void set_visualization_mode(TerrainVisualizationMode mode) { visualization_mode = mode; }; // switch between shading and debug views
};

inline void Terrain::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	material.bind(
		command_buffer,
		current_frame,
		{
			m_model,
			mesh.get_vertex_address(),
			tesselation_strength,
			max_tesselation,
			texture_eps,
			visualization_mode,
			m_cascade_idx
		}
	);

	mesh.draw(command_buffer, current_frame);
}