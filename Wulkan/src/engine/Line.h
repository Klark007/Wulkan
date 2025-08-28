#pragma once

#include "common.h"
#include "Mesh.h"
#include "Renderpass.h"

class Line : public Shape
{
public:
	Line() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const glm::vec4 color);
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const std::vector<glm::vec4>& colors);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers);
	
	// creates singleton render pass, needs to be deleted by caller of function
	static RenderPass<PushConstants, 1> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 1>& layouts, Texture& color_rt, Texture& depth_rt);

	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
protected:
	std::vector<Vertex> vertices;
	VKW_Buffer vertex_buffer;
	VkDeviceAddress vertex_address;

	MaterialInstance<PushConstants, 1> material;

	VKW_Buffer index_buffer;
	uint32_t nr_indices;
public:
	// update vertex position. Needs to be the same length as original vertices
	void update_vertices(const std::vector<glm::vec3>& points);
};

inline void Line::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
	material.bind(
		command_buffer,
		current_frame,
		{
			m_model,
			m_inv_model,
			vertex_address,
			m_cascade_idx
		}
	);

	// bind index buffer and draw
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(command_buffer, nr_indices, 1, 0, 0, 0);
}
