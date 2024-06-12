#include "Camera.h"

Camera::Camera()
{
	position = glm::vec3(0.0, 2.0, 10.0);
	up = glm::vec3(0.0, 1.0, 0.0);
	direction = glm::vec3(0.0, -1.0, 0.0);
}

Camera::Camera(glm::vec3 pos, glm::vec3 des, glm::vec3 up)
	: position{ pos }, direction{ glm::normalize(des-pos) }, up{glm::normalize(up)} {}

glm::mat4 Camera::generate_view_mat()
{
	return glm::lookAt(position, position + direction, up);
}
