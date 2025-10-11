#include "Camera.h"

Camera::Camera(glm::vec3 pos, glm::vec3 des, unsigned int res_x, unsigned int res_y, float focx, float focy, float near_plane, float far_plane)
	: Camera(pos, des, res_x, res_y, std::atanf(static_cast<float>(res_y) / focy * 0.5f) * 2, near_plane, far_plane)
{
	foc_x = focx;
	foc_y = focy;
}

Camera::Camera(glm::vec3 pos, glm::vec3 des, unsigned int res_x, unsigned int res_y, float fov, float near_plane, float far_plane)
	: position{ pos },
	res_x{ res_x },
	res_y{ res_y },
	fov{ fov },
	ortho_height {0},
	m_z_near{ near_plane },
	m_z_far{ far_plane }
{
	set_dir(des - pos);
}