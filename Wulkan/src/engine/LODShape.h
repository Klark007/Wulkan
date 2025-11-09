#pragma once
#include "Shape.h"
#include <type_traits>
#include "InstancedLODShape.h"

template <typename T> requires std::is_base_of_v<Shape, T>
class LODShape : public Shape
{
public:
	LODShape() = default;

	// expects highest definition mesh first, lowest last
	// before will need to have called set_descriptor_bindings
	// if ratios are left empty (default) the LOD choice will be distributed equally in distance
	void init(std::vector<T >&& shapes, std::vector<float> ratios = {});
	void del() override;

	virtual void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
protected:
	std::vector<T> m_shapes;
	std::vector<float> m_ratios;
	uint32_t m_lod_levels;

	glm::vec3 m_camera_position;
	glm::vec3 m_camera_direction;
	float m_near_plane;
	float m_far_plane;

	uint32_t get_lod_level(uint32_t instance = 0);
public:
	inline void set_camera_info(const glm::vec3& pos, const glm::vec3& dir, float near_plane, float far_plane);

	inline void set_model_matrix(const glm::mat4& m) override;
	void set_cascade_idx(int idx) override;

	inline void set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses) override;
	inline void set_instance_count(uint32_t count) override;
	inline void set_visualization_mode(VisualizationMode mode) override;
	glm::vec3 get_instance_position(uint32_t instance = 0) override;
};

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::init(std::vector<T > && shapes, std::vector<float> ratios) {
	if (!(ratios.empty() || ratios.size() == shapes.size())) {
		throw RuntimeException(fmt::format("Tried to initialize LODShape with ratios of length {} for {} many LOD levels", ratios.size(), shapes.size()), __FILE__, __LINE__);
	}
	m_shapes = shapes;

	m_lod_levels = m_shapes.size();
	if (ratios.empty()) {
		for (uint32_t i = 1; i <= m_lod_levels; i++) {
			m_ratios.push_back(((float)i) / m_lod_levels);
		}
	}
	else {
		m_ratios = ratios;
	}
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::del()
{
	for (T& s : m_shapes)
		s.del();
}

template<typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{	
	m_shapes[get_lod_level()].draw(command_buffer, current_frame);
}

template<typename T> requires std::is_base_of_v<Shape, T>
inline uint32_t LODShape<T>::get_lod_level(uint32_t instance)
{
	// project onto camera dir and see distance (crorresponds to distance to plane)
	float distance = glm::dot(m_camera_direction, get_instance_position(instance) - m_camera_position);
	// treat negative distances also as far away (TODO: Shouldn't matter if culling were supported)
	float distance_01 = map(std::clamp(abs(distance), m_near_plane, m_far_plane), m_near_plane, m_far_plane, 0, 1);

	uint32_t lod_idx = 0;
	while (distance_01 > m_ratios[lod_idx]) {
		lod_idx++;
		if (lod_idx == m_lod_levels) {
			lod_idx--;
			break;
		}
	}

	return lod_idx;
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_camera_info(const glm::vec3& pos, const glm::vec3& dir, float near_plane, float far_plane)
{
	m_camera_position = pos;
	m_camera_direction = dir;
	m_near_plane = near_plane;
	m_far_plane = far_plane;
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_model_matrix(const glm::mat4& m)
{
	Shape::set_model_matrix(m);
	for (T& s : m_shapes)
		s.set_model_matrix(m);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_cascade_idx(int idx)
{
	for (T& s : m_shapes)
		s.set_cascade_idx(idx);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_visualization_mode(VisualizationMode mode)
{
	for (T& s : m_shapes)
		s.set_visualization_mode(mode);
}

template<typename T> requires std::is_base_of_v<Shape, T>
inline glm::vec3 LODShape<T>::get_instance_position(uint32_t instance)
{
	assert(instance < m_instance_count && "Attempt to get position with invalid instance");
	return glm::vec3(m_model[3]);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_instance_buffer_address(const std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT>& addresses)
{
	throw RuntimeException("set_instance_buffer_address not supported for LODMesh. If we want Instanced LOD: create an InstancedLODShape<InstancedShape<T>>", __FILE__, __LINE__);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_instance_count(uint32_t count)
{
	throw RuntimeException("set_instance_count not supported for LODMesh. If we want Instanced LOD: create an InstancedLODShape<InstancedShape<T>>", __FILE__, __LINE__);
}