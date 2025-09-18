#pragma once

#include "Mesh.h"
#include "Renderpass.h"

constexpr size_t OBJ_MESH_DESC_SET_COUNT = 3;

struct PBRUniform {
	alignas(16) glm::vec3 diffuse;
	alignas(4) float metallic;

	alignas(16) glm::vec3 specular;
	alignas(4) float roughness;

	alignas(16) glm::vec3 emission;
	alignas(4) float eta;
	alignas(4) uint32_t configuration; // 0 bit: if true read from albedo texture
};

class ObjMesh : public Shape
{
public:
	ObjMesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, OBJ_MESH_DESC_SET_COUNT>& render_pass, const std::string& obj_path, const std::string& mtl_path="./models/");
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& general_ubo, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& shadow_map_ubo, Texture& shadow_map, const VKW_Sampler& shadow_map_sampler, const VKW_Sampler& shadow_map_gather_sampler);
	void del() override;

	// TODO: could be kept seperate
	// depth_bias only works in depth_only mode
	static RenderPass<PushConstants, OBJ_MESH_DESC_SET_COUNT> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, OBJ_MESH_DESC_SET_COUNT>& layouts, Texture& color_rt, Texture& depth_rt, bool depth_only = false, bool bias_depth = false);
	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	// goes over all materials in obj and renders them, expects to be in active command buffer
	// TODO: Current assumption is that all materials in ObjMesh use the same pipeline
	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	std::vector<Mesh> m_meshes; // one mesh per material
	std::vector<MaterialInstance<PushConstants, OBJ_MESH_DESC_SET_COUNT>> m_materials;
	VKW_Buffer m_vertex_buffer;
	VkDeviceAddress m_vertex_buffer_address;

	// TODO: also seperate
	std::array<std::vector<VKW_Buffer>, MAX_FRAMES_IN_FLIGHT> m_uniform_buffers; // per frame, per material uniform buffer's
};


inline void ObjMesh::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	for (size_t i = 0; i < m_materials.size(); i++) {
		m_materials[i].bind(
			command_buffer,
			current_frame,
			{
				m_model,
				m_inv_model,
				m_meshes[i].get_vertex_address(),
				m_cascade_idx
			}
		);

		m_meshes[i].draw(command_buffer, current_frame);
	}
}