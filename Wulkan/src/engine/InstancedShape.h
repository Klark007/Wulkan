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
	// If per_instance_data is empty then dynamically update the m_instance_buffer
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, T&& shape, uint32_t instance_count, const std::vector<InstanceData>& per_instance_data);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	T m_shape;
	std::vector<InstanceData> m_instance_data;
	VKW_Buffer m_instance_buffer;
	bool m_can_update;
	uint32_t m_max_instance_count; // the number of instances m_instance_buffer supports. Current instance_count can be lower 
public:
	void update_instance_data(const std::vector<InstanceData>& per_instance_data);

	void set_model_matrix(const glm::mat4& m) override { m_shape.set_model_matrix(m); };
	void set_cascade_idx(int idx) override { m_shape.set_cascade_idx( idx); };

	inline void set_visualization_mode(VisualizationMode mode) { m_shape.set_visualization_mode(mode); };
	inline virtual virtual void set_instance_count(uint32_t count) override;
	virtual void set_instance_buffer_address(VkDeviceAddress address) { m_shape.set_instance_buffer_address(address); };
	glm::vec3 get_instance_position(uint32_t instance = 0) override;
};

template<typename T>  requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, T&& shape, uint32_t instance_count, const std::vector<InstanceData>& per_instance_data)
{
	m_shape = shape;
	m_instance_data = per_instance_data;
	m_max_instance_count = instance_count;

	if (!(m_instance_data.empty() || m_instance_data.size() == m_max_instance_count)) {
		throw RuntimeException(fmt::format("Tried to initialize InstancedShape with {} many instances but {} per_instance_data", instance_count, per_instance_data.size()), __FILE__, __LINE__);
	}

	VkDeviceSize instance_buffer_size = sizeof(InstanceData) * m_max_instance_count;
	if (!m_instance_data.empty()) {
		// copy per instance data into a buffer
		VKW_Buffer instance_staging_buffer = create_staging_buffer(&device, instance_buffer_size, m_instance_data.data(), instance_buffer_size, "Instance staging buffer");

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
	}
	else {
		m_instance_buffer.init(
			&device,
			instance_buffer_size,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			sharing_exlusive(),
			true,
			"Instance buffer"
		);
		m_instance_buffer.map();

		m_can_update = true;
	}

	// get instance buffer address
	VkBufferDeviceAddressInfo address_info{};
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.buffer = m_instance_buffer;

	set_instance_buffer_address(vkGetBufferDeviceAddress(device, &address_info));
	set_instance_count(m_max_instance_count);
}

// overhead cost of virtual function call
template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	m_shape.draw(command_buffer, current_frame);
}

// TODO: fix Costs two copies of per_instance_data
template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::update_instance_data(const std::vector<InstanceData>& per_instance_data)
{
	assert(m_can_update && "InstancedShape needs to be initalized with empty m_instance_data to make updatable");
	assert(per_instance_data.size() <= m_instance_count && "per_instance_data size needs to be at least");
	
	m_instance_data = per_instance_data;
	set_instance_count(m_instance_data.size());

	m_instance_buffer.copy(m_instance_data.data(), sizeof(InstanceData) * m_instance_data.size());
}

template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedShape<T>::set_instance_count(uint32_t count)
{
	m_instance_count = count;
	m_shape.set_instance_count(count);
}

template<typename T> requires std::is_base_of_v<Shape, T>
inline glm::vec3 InstancedShape<T>::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_count && "Attempt to get position with invalid instance");
	return glm::vec3(m_model[3]) + m_instance_data[instance].position; 
}

template<typename T> requires std::is_base_of_v<Shape, T>
void InstancedShape<T>::del()
{
	m_shape.del();
	m_instance_buffer.del();
}