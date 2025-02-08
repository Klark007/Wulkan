#include "Mesh.h"

void Mesh::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices)
{
	// create gpu side buffer storing vertices
	VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();
	VKW_Buffer vertex_staging_buffer = create_staging_buffer(&device, vertex_buffer_size, vertices.data(), vertex_buffer_size);

	vertex_buffer.init(
		&device,
		vertex_buffer_size,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		sharing_exlusive(),
		false
	);

	vertex_buffer.copy(&transfer_pool, vertex_staging_buffer);
	vertex_staging_buffer.del();


	// create gpu side buffer storing indices
	VkDeviceSize index_buffer_size = sizeof(uint32_t) * indices.size();
	VKW_Buffer index_staging_buffer = create_staging_buffer(&device, index_buffer_size, indices.data(), index_buffer_size);

	index_buffer.init(
		&device,
		index_buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		sharing_exlusive(),
		false
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

void Mesh::del()
{
	vertex_buffer.del();
	index_buffer.del();
}
