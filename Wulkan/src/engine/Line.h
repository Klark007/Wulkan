#pragma once

#include "common.h"
#include "Mesh.h"
#include "Material.h"

class Line : public Shape
{
public:
	Line() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, MaterialType<PushConstants, 1>& material_type, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const glm::vec4 color);
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, MaterialType<PushConstants, 1>& material_type, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const std::vector<glm::vec4>& colors);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers);
	
	//static VKW_GraphicsPipeline create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, SharedLineData& shared_line_data);
	// creates singleton material type, needs to be deleted by caller of function
	static MaterialType<PushConstants, 1> create_material_type(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 1>& layouts, Texture& color_rt, Texture& depth_rt);

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
			vertex_address,
			model
		}
	);

	// bind index buffer and draw
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(command_buffer, nr_indices, 1, 0, 0, 0);
}

inline MaterialType<PushConstants, 1> Line::create_material_type(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 1>& layouts, Texture& color_rt, Texture& depth_rt)
{
	MaterialType<PushConstants, 1> material{};

	// Push constants
	VKW_PushConstant<PushConstants> push_constant;
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT, 0);

	// Graphics pipeline
	VKW_Shader vert_shader{};
	vert_shader.init(device, "shaders/line/line_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "Line vertex shader");

	VKW_Shader frag_shader{};
	frag_shader.init(device, "shaders/line/line_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "Line fragment shader");

	VKW_GraphicsPipeline graphics_pipeline{};

	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	graphics_pipeline.set_culling_mode();
	graphics_pipeline.enable_depth_test();
	graphics_pipeline.enable_depth_write();

	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });

	graphics_pipeline.add_descriptor_sets(layouts);
	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	graphics_pipeline.init(device, "Line graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	material.init(
		graphics_pipeline,
		layouts,
		push_constant,
		"Line material"
	);

	return material;
}
