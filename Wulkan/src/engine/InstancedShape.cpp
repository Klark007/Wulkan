#include "InstancedShape.h"

void InstancedShape::init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const VKW_Path& obj_path, const std::vector<InstanceData>& per_instance_data, const VKW_Path& mtl_path)
{
	ObjMesh::init(device, graphics_pool, transfer_pool, descriptor_pool, render_pass, obj_path, mtl_path);

	// copy per instance data into a buffer
	VkDeviceSize instance_buffer_size = sizeof(InstanceData) * per_instance_data.size();
	VKW_Buffer instance_staging_buffer = create_staging_buffer(&device, instance_buffer_size, per_instance_data.data(), instance_buffer_size, "Instance staging buffer");

	instance_buffer.init(
		&device,
		instance_buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		sharing_exlusive(),
		false,
		"Instance buffer"
	);

	instance_buffer.copy(&transfer_pool, instance_staging_buffer);
	instance_staging_buffer.del();

	// get instance buffer address
	VkBufferDeviceAddressInfo address_info{};
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.buffer = instance_buffer;
	
	m_instance_buffer_address = vkGetBufferDeviceAddress(device, &address_info);
	set_instance_count(per_instance_data.size());
}

void InstancedShape::del()
{
	ObjMesh::del();
	instance_buffer.del();
}
