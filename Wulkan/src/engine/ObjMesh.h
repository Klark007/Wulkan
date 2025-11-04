#pragma once

#include "PBRMesh.h"

class ObjMesh : public PBRMesh
{
public:
	ObjMesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const VKW_Path& obj_path, const VKW_Path& mtl_path="");
	void del() override;

	// goes over all materials in obj and renders them, expects to be in active command buffer
	// TODO: Current assumption is that all materials in ObjMesh use the same pipeline
	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
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
				m_instance_buffer_address,
				m_cascade_idx
			}
		);

		m_meshes[i].draw(command_buffer, current_frame);
	}
}