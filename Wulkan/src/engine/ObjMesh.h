#pragma once

#include "Path.h"
#include "Mesh.h"
#include "Renderpass.h"
#include "PBRMaterial.h"

class ObjMesh : public Shape
{
public:
	ObjMesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const VKW_Path& obj_path, const VKW_Path& mtl_path="");
	void set_descriptor_bindings(Texture& texture_fallback, const VKW_Sampler& general_sampler);
	void del() override;

	// TODO: could be kept seperate (Other file formats should use same render_pass types (different from eg terrain)
	// bias_depth only works in depth_only mode
	static RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, PBR_MAT_DESC_SET_COUNT>& layouts, Texture& color_rt, Texture& depth_rt, bool depth_only = false, bool bias_depth = false);
	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	// goes over all materials in obj and renders them, expects to be in active command buffer
	// TODO: Current assumption is that all materials in ObjMesh use the same pipeline
	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	std::vector<Mesh> m_meshes; // one mesh per material
	
	inline static VKW_DescriptorSetLayout descriptor_set_layout;
	std::vector<PBRMaterial> m_materials;
	
	VKW_Buffer m_vertex_buffer;
	VkDeviceAddress m_vertex_buffer_address = 0;
public:
	inline void set_visualization_mode(PBRVisualizationMode mode);
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

inline void ObjMesh::set_visualization_mode(PBRVisualizationMode mode)
{
	for (PBRMaterial& mat : m_materials) {
		mat.set_visualization_mode(mode);
	}
}
