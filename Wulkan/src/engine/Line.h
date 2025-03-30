#pragma once

#include "common.h"
#include "Mesh.h"

class SharedLineData : public VKW_Object {
public:
	SharedLineData() = default;
	void init(const VKW_Device* device);
	void del() override;
private:
	VKW_DescriptorSetLayout descriptor_set_layout;
	VKW_PushConstant<PushConstants> push_constant;
public:
	const VKW_DescriptorSetLayout& get_descriptor_set_layout() const { return descriptor_set_layout; };

	inline VKW_PushConstant<PushConstants>& get_pc() { return push_constant; }
	inline std::vector<VkPushConstantRange> get_push_consts_range() const {
		return { push_constant.get_range() };
	}
};

class Line : public Shape
{
public:
	Line() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedLineData* shared_line_data, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, glm::vec4 color);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers);
	static VKW_GraphicsPipeline create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedLineData& shared_line_data);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
protected:
	std::vector<Vertex> vertices;
	VKW_Buffer vertex_buffer;
	VkDeviceAddress vertex_address;

	VKW_Buffer index_buffer;
	uint32_t nr_indices;

	SharedLineData* shared_data;
	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
public:
	// update vertex position. Needs to be the same length as original vertices
	void update_vertices(const std::vector<glm::vec3>& points);
};

inline void Line::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
	descriptor_sets.at(current_frame).bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout());

	shared_data->get_pc().update({
		vertex_address,
		model
	});
	shared_data->get_pc().push(command_buffer, pipeline.get_layout());

	// bind index buffer and draw
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(command_buffer, nr_indices, 1, 0, 0, 0);
}

inline VKW_GraphicsPipeline Line::create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedLineData& shared_line_data)
{
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

	graphics_pipeline.add_descriptor_sets({ shared_line_data.get_descriptor_set_layout() });
	graphics_pipeline.add_push_constants(shared_line_data.get_push_consts_range());

	graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	graphics_pipeline.init(device, "Line graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	return graphics_pipeline;
}
