#pragma once

#include "common.h"

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
	VkDeviceAddress vertex_address;

	VKW_Buffer index_buffer;
	uint32_t nr_indices;
public:
	void set_vertex_address(VkDeviceAddress address) { vertex_address = address; };
	VkDeviceAddress get_vertex_address() const { return vertex_address; };
};

inline void Mesh::draw(const VKW_CommandBuffer& command_buffer, uint32_t)
{
	// bind index buffer
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(command_buffer, nr_indices, 1, 0, 0, 0);
}

// initializes a vertex buffer and also sets it's device address
void create_vertex_buffer(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::vector<Vertex>& vertices, VKW_Buffer& vertex_buffer, VkDeviceAddress& vertex_address);
