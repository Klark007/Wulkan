#pragma once

#include "Shape.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_Buffer.h"

#include <glm/glm.hpp>

#include <optional>

// uv interleaved due to GPU allignment 
struct Vertex {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

class Mesh : public Shape
{
public:
	Mesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	// can also be initialized without owning the vertex buffer (i.e. shared between multiple meshes)
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VkDeviceAddress vert_addr, const std::vector<uint32_t>& indices);
	void del() override;

	// binds the indices and calls vkCmdDrawIndexed, expects to be in active command buffer
	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
protected:
	std::optional<VKW_Buffer> vertex_buffer;
	VkDeviceAddress vertex_address{};

	VKW_Buffer index_buffer;
	uint32_t nr_indices = 0;
public:
	void set_vertex_address(VkDeviceAddress address) { vertex_address = address; };
	VkDeviceAddress get_vertex_address() const { return vertex_address; };

	void set_model_matrix(const glm::mat4& m) override { m_model = m; };
	void set_cascade_idx(int idx) override { m_cascade_idx = idx; };
	void set_lod_level(int lod_level) override { m_lod_level = lod_level; };
	inline void set_visualization_mode(VisualizationMode mode) override;
	void set_instance_count(uint32_t count) { m_instance_count = count; };
	inline  void set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses) override;
	glm::vec3 get_instance_position(uint32_t instance = 0) override;
};

inline void Mesh::draw(const VKW_CommandBuffer& command_buffer, uint32_t)
{
	// bind index buffer
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(command_buffer, nr_indices, m_instance_count, 0, 0, 0);
}

inline void Mesh::set_visualization_mode(VisualizationMode mode)
{
	throw NotImplementedException("set_visualization_mode not implemented for Mesh class", __FILE__, __LINE__);
}

inline void Mesh::set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses)
{
	throw NotImplementedException("set_instance_buffer_address not implemented for Mesh class", __FILE__, __LINE__);
}

inline glm::vec3 Mesh::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_count && "Attempt to get position with invalid instance");
	return glm::vec3(m_model[3]);
}

// initializes a vertex buffer and also sets it's device address
void create_vertex_buffer(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::vector<Vertex>& vertices, VKW_Buffer& vertex_buffer, VkDeviceAddress& vertex_address);
