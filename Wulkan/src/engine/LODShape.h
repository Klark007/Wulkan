#pragma once
#include "Shape.h"
#include <type_traits>

template <typename T> requires std::is_base_of_v<Shape, T>
class LODShape : public Shape
{
public:
	LODShape() = default;

	// expects highest definition mesh first, lowest last
	// before will need to have called set_descriptor_bindings
	void init(std::vector<T > && shapes);
	void del() override;

	void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	std::vector<T> m_shapes;
	
	glm::vec3 m_camera_position;
	float m_near_plane;
	float m_far_plane;
public:
	inline void set_camera_info(const glm::vec3& pos, float near, float far);

	inline void set_model_matrix(const glm::mat4& m) override;
	void set_cascade_idx(int idx) override;

	inline void set_instance_buffer_address(VkDeviceAddress address) override;
	inline void set_instance_count(uint32_t count) override;
	inline void set_visualization_mode(VisualizationMode mode) override;

	int idx;
};

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::init(std::vector<T > && shapes) {
	m_shapes = shapes;
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
	m_shapes[idx].draw(command_buffer, current_frame);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_camera_info(const glm::vec3& pos, float near, float far)
{
	m_camera_position = pos;
	m_near_plane = near;
	m_far_plane = far;
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

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_instance_buffer_address(VkDeviceAddress address)
{
	throw RuntimeException("set_instance_buffer_address not supported for LODMesh. If we want Instanced LOD: create an LODShape<InstancedShape<T>>", __FILE__, __LINE__);
}

template <typename T> requires std::is_base_of_v<Shape, T>
inline void LODShape<T>::set_instance_count(uint32_t count)
{
	throw RuntimeException("set_instance_count not supported for LODMesh. If we want Instanced LOD: create an LODShape<InstancedShape<T>>", __FILE__, __LINE__);
}