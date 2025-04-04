#include "Line.h"

void SharedLineData::init(const VKW_Device* device)
{
	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);
	descriptor_set_layout.init(device, "Line Desc Layout");

	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT, 0);
}

void SharedLineData::del()
{
	descriptor_set_layout.del();
}

void Line::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedLineData* shared_line_data, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const glm::vec4 color)
{
	std::vector<glm::vec4> colors{ points.size(), color };
	init(device, transfer_pool, descriptor_pool, shared_line_data, points, indices, colors);
}

void Line::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedLineData* shared_line_data, const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const std::vector<glm::vec4>& colors) {
	shared_data = shared_line_data;
	
	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.init(&device, &descriptor_pool, shared_data->get_descriptor_set_layout(), "Line Desc Set");
	}

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

	nr_indices = indices.size();
}

void Line::update_vertices(const std::vector<glm::vec3>& points)
{
	if (points.size() != vertices.size()) {
		throw SetupException(std::format("Tried to set line vertices wth original size {} to vertices with size {}", vertices.size(), points.size()), __FILE__, __LINE__);
	}

	for (int i = 0; i < points.size(); i++) {
		vertices.at(i) = { points.at(i), vertices.at(i).uv_x, vertices.at(i).normal, vertices.at(i).uv_y, vertices.at(i).color};
	}

	VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();
	vertex_buffer.copy(vertices.data(), vertex_buffer_size);
}

void Line::set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers)
{
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set = descriptor_sets.at(i);

		set.update(0, uniform_buffers.at(i));
	}
}

void Line::del()
{
	for (VKW_DescriptorSet& set : descriptor_sets) {
		set.del();
	}

	vertex_buffer.del();
	index_buffer.del();
}