#pragma once

#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_GraphicsPipeline.h"

enum class VisualizationMode {
	Shaded,
	Normals,
	Diffuse,
	ShadowCascade,
	LODLevel,
};

class Shape : public VKW_Object {
public:
	inline virtual void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) = 0;
	virtual void del() override = 0;
protected:
	// one should use setter functions instead of writing directly to this
	glm::mat4 m_model = glm::mat4(1);
	glm::mat4 m_inv_model = glm::mat4(1);
	int m_cascade_idx = 0;
	int m_lod_level = 0;
	std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT> m_instance_buffer_addresses = {0};

	// should only be set by set_instance_count
	uint32_t m_instance_count = 1;
public:
	virtual void set_model_matrix(const glm::mat4& m) = 0;
	virtual void set_cascade_idx(int idx) = 0;
	virtual void set_lod_level(int lod_level) = 0;

	virtual void set_visualization_mode(VisualizationMode mode) = 0;
	
	// might be different for shapes composed of multiple shapes (see ObjMesh)
	virtual void set_instance_count(uint32_t count) = 0;
	virtual void set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses) = 0;
	virtual glm::vec3 get_instance_position(uint32_t instance = 0) = 0;

};

inline void Shape::set_model_matrix(const glm::mat4& m)
{
	m_model = m;
	m_inv_model = glm::inverse(m_model);
}