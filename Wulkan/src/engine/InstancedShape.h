#pragma once
#include "common.h"
#include "ObjMesh.h"
#include <type_traits>

struct InstanceData {
	alignas(16) glm::vec3 position;
};

template <typename T> requires std::is_base_of_v<Shape, T>
class InstancedShape : public Shape
{
public:
	// before will need to have called set_descriptor_bindings
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, T&& shape, const std::vector<InstanceData>& per_instance_data);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	T m_shape;
	VKW_Buffer m_instance_buffer;
public:
	void set_model_matrix(const glm::mat4& m) override { m_shape.set_model_matrix(m); };
	void set_cascade_idx(int idx) override { m_shape.set_cascade_idx( idx); };

	virtual void set_instance_buffer_address(VkDeviceAddress address) { m_shape.set_instance_buffer_address(address); };
	inline void set_visualization_mode(VisualizationMode mode) { m_shape.set_visualization_mode(mode); };
	inline virtual virtual void set_instance_count(uint32_t count) override { m_shape.set_instance_count(count); };
};

template<typename T>  requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, T&& shape, const std::vector<InstanceData>& per_instance_data)
{
	m_shape = shape;

	// copy per instance data into a buffer
	VkDeviceSize instance_buffer_size = sizeof(InstanceData) * per_instance_data.size();
	VKW_Buffer instance_staging_buffer = create_staging_buffer(&device, instance_buffer_size, per_instance_data.data(), instance_buffer_size, "Instance staging buffer");

	m_instance_buffer.init(
		&device,
		instance_buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		sharing_exlusive(),
		false,
		"Instance buffer"
	);

	m_instance_buffer.copy(&transfer_pool, instance_staging_buffer);
	instance_staging_buffer.del();

	// get instance buffer address
	VkBufferDeviceAddressInfo address_info{};
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.buffer = m_instance_buffer;

	set_instance_buffer_address(vkGetBufferDeviceAddress(device, &address_info));
	set_instance_count(per_instance_data.size());
}

// overhead cost of virtual function call
template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	m_shape.draw(command_buffer, current_frame);
}

template<typename T> requires std::is_base_of_v<Shape, T>
void InstancedShape<T>::del()
{
	m_shape.del();
	m_instance_buffer.del();
}