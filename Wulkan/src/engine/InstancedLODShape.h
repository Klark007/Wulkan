#pragma once

#include "Shape.h"
#include "InstancedShape.h"
#include "LODShape.h"

#include <type_traits>

template <typename T> requires std::is_base_of_v<Shape, T>
class InstancedLODShape : public LODShape<InstancedShape<T>> {
public:
	// expects highest definition mesh first, lowest last
	// before will need to have called set_descriptor_bindings
	// if ratios are left empty (default) the LOD choice will be distributed equally in distance
	void init(std::vector<InstancedShape<T>>&& shapes, const std::vector<InstanceData>& per_instance_data, std::vector<float> ratios = {});

	void update(uint32_t current_frame);
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
	m_per_lod_instance_data.resize(this->m_lod_levels);
}


template<typename T> requires std::is_base_of_v<Shape, T>
inline void InstancedLODShape<T>::update(uint32_t current_frame)
{
	// needs explicit this due to templated base class
	for (uint32_t i = 0; i < m_instance_data.size(); i++) {
		InstanceData data = m_instance_data[i];
		uint32_t lod_level = this->get_lod_level(i);
		m_per_lod_instance_data[lod_level].push_back({ data });
	}

	for (uint32_t i = 0; i < this->m_lod_levels; i++) {
		this->m_shapes[i].update_instance_data(m_per_lod_instance_data[i], current_frame);
		// clear for next frame's draw
		m_per_lod_instance_data[i].clear();
	}
}

template<typename T>  requires std::is_base_of_v<Shape, T>
inline void InstancedLODShape<T>::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	for (uint32_t i = 0; i < this->m_lod_levels; i++) {
		this->m_shapes[i].draw(command_buffer, current_frame);	
	}
}

template<typename T>  requires std::is_base_of_v<Shape, T>
inline glm::vec3 InstancedLODShape<T>::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_data.size() && "Attempt to get position with invalid instance");
	return glm::vec3(this->m_model[3]) + m_instance_data[instance].position;
}
