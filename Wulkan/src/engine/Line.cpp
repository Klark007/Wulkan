#include "Line.h"
#include <iostream>

void Line::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const glm::vec4 color)
{
	std::vector<glm::vec4> colors{ points.size(), color };
	init(device, transfer_pool, descriptor_pool, render_pass, points, indices, colors);
}

void Line::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const std::vector<glm::vec4>& colors) {
	material.init(device, descriptor_pool, render_pass, {}, {}, "Line material");

	vertices.resize(points.size());

	for (int i = 0; i < points.size(); i++) {
		vertices.at(i) = { points.at(i), 0, glm::vec3(0,0,0), 0, colors.at(i)};
	}

	// in contrast to Mesh, we want to be able to update vertices (but not indices) on a per frame basis
	VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();

	vertex_buffer.init(
		&device,
		vertex_buffer_size,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		sharing_exlusive(),
		true,
		"Line vertex buffer"
	);
	vertex_buffer.map();
	vertex_buffer.copy(vertices.data(), vertex_buffer_size);

	VkDeviceSize index_buffer_size = sizeof(uint32_t) * indices.size();
	VKW_Buffer index_staging_buffer = create_staging_buffer(&device, index_buffer_size, indices.data(), index_buffer_size, "Index staging buffer");

	index_buffer.init(
		&device,
		index_buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		sharing_exlusive(),
		false,
		"Mesh index buffer"
	);

	index_buffer.copy(&transfer_pool, index_staging_buffer);
	index_staging_buffer.del();

	// get device address
	VkBufferDeviceAddressInfo address_info{};
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.buffer = vertex_buffer;

	vertex_address = vkGetBufferDeviceAddress(device, &address_info);

	nr_indices = static_cast<uint32_t>(indices.size());
}

void Line::update_vertices(const std::vector<glm::vec3>& points)
{
	if (points.size() != vertices.size()) {
		throw SetupException(fmt::format("Tried to set line vertices wth original size {} to vertices with size {}", vertices.size(), points.size()), __FILE__, __LINE__);
	}

	for (int i = 0; i < points.size(); i++) {
		vertices.at(i) = { points.at(i), vertices.at(i).uv_x, vertices.at(i).normal, vertices.at(i).uv_y, vertices.at(i).color};
	}

	VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();
	vertex_buffer.copy(vertices.data(), vertex_buffer_size);
}

RenderPass<PushConstants, 1> Line::create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 1>& layouts, Texture& color_rt, Texture& depth_rt)
{
	RenderPass<PushConstants, 1> render_pass{};

	// Push constants
	VKW_PushConstant<PushConstants> push_constant{};
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

	render_pass.init(
		graphics_pipeline,
		layouts,
		push_constant
	);

	return render_pass;
}

void Line::del()
{
	vertex_buffer.del();
	index_buffer.del();

	material.del();
}