#pragma once

#include "Shape.h"
#include "InstancedShape.h"
#include "LODShape.h"

#include <type_traits>
#include <spdlog/spdlog.h>

template <typename T> requires std::is_base_of_v<Shape, T>
class InstancedLODShape : public LODShape<InstancedShape<T>> {
public:
	// expects highest definition mesh first, lowest last
	// before will need to have called set_descriptor_bindings
	// if ratios are left empty (default) the LOD choice will be distributed equally in distance
	void init(std::vector<InstancedShape<T>>&& shapes, const std::vector<InstanceData>& per_instance_data, std::vector<float> ratios = {});

	void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	std::vector<InstanceData> m_instance_data;
	std::vector<std::vector<InstanceData>> m_per_lod_instance_data;
	glm::vec3 get_instance_position(uint32_t instance = 0) override;
};

template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedLODShape<T>::init(std::vector<InstancedShape<T>>&& shapes, const std::vector<InstanceData>& per_instance_data, std::vector<float> ratios)
{
	LODShape<InstancedShape<T>>::init(std::move(shapes), ratios);

	m_instance_data = per_instance_data;
	m_per_lod_instance_data.resize(m_instance_data.size());
}

template<typename T>  requires std::is_base_of_v<Shape, T>
inline void InstancedLODShape<T>::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	// needs explicit this due to templated base class
	this->m_shapes[this->get_lod_level()].draw(command_buffer, current_frame);
}

template<typename T>  requires std::is_base_of_v<Shape, T>
inline glm::vec3 InstancedLODShape<T>::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_data.size() && "Attempt to get position with invalid instance");
	return glm::vec3(this->m_model[3]) + m_instance_data[instance].position;
}
