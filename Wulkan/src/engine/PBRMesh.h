#pragma once

#include "Path.h"
#include "Mesh.h"
#include "Renderpass.h"
#include "PBRMaterial.h"

class PBRMesh : public Shape {
public:
	void set_descriptor_bindings(Texture& texture_fallback, const VKW_Sampler& general_sampler) ;
	virtual void del() override = 0;

	// TODO: could be kept seperate (Other file formats should use same render_pass types (different from eg terrain)
	// bias_depth only works in depth_only mode
	static RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, PBR_MAT_DESC_SET_COUNT>& layouts, Texture& color_rt, Texture& depth_rt, bool depth_only = false, bool bias_depth = false, bool cull_backfaces = true);
	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	// goes over all materials in obj and renders them, expects to be in active command buffer
	// TODO: Current assumption is that all materials in ObjMesh use the same pipeline
	inline virtual void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override = 0;
protected:
	std::vector<Mesh> m_meshes; // usually: one mesh per material
	std::vector<PBRMaterial> m_materials;

	inline static VKW_DescriptorSetLayout descriptor_set_layout;
	VKW_Buffer m_vertex_buffer;

	VkDeviceAddress m_vertex_buffer_address = 0;
public:
	void set_model_matrix(const glm::mat4& m) override { m_model = m; };
	void set_cascade_idx(int idx) override { m_cascade_idx = idx; };
	inline void set_visualization_mode(VisualizationMode mode) override;
	void set_lod_level(int lod_level) override { m_lod_level = lod_level; };

	inline void set_instance_count(uint32_t count) override;
	void set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses) override { m_instance_buffer_addresses = addresses; };
	glm::vec3 get_instance_position(uint32_t instance = 0) override;
};

inline void PBRMesh::set_visualization_mode(VisualizationMode mode)
{
	for (PBRMaterial& mat : m_materials) {
		mat.set_visualization_mode(mode);
	}
}

inline void PBRMesh::set_instance_count(uint32_t count)
{
	m_instance_count = count;
	for (Mesh& m : m_meshes)
		m.set_instance_count(m_instance_count);
}

inline glm::vec3 PBRMesh::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_count && "Attempt to get position with invalid instance");
	return glm::vec3(m_model[3]);
}
