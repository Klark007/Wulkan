#pragma once

#include "vk_types.h"

#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_Buffer.h"

#include <glm/glm.hpp>

// uv interleaved due to GPU allignment 
struct Vertex {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

class Mesh : VKW_Object
{
public:
	Mesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices);
	void del() override;

	// binds the indices and calls vkCmdDrawIndexed, expects to be in active command buffer
	void draw(const VKW_CommandBuffer& command_buffer) const;
protected:
	VKW_Buffer vertex_buffer;
	VkDeviceAddress vertex_address;

	VKW_Buffer index_buffer;
	uint32_t nr_indices;
public:
	VkDeviceAddress get_vertex_address() const { return vertex_address; };
};

