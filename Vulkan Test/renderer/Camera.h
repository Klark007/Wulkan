#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 pos, glm::vec3 des, glm::vec3 up);

	inline void set_pos(glm::vec3 pos) { position = pos; }
	inline void look_at(glm::vec3 des) { direction = glm::normalize(des - position); }
	inline void set_up(glm::vec3 up) { up = glm::normalize(up);  }
	inline void set_dir(glm::vec3 dir) { direction = glm::normalize(dir); };
private:
	glm::vec3 position;
	glm::vec3 up;
	glm::vec3 direction;

public:
	glm::mat4 generate_view_mat();
	inline glm::vec3 get_pos() { return position; };
	inline glm::vec3 get_des() { return position + direction; };
	inline glm::vec3 get_dir() { return direction; };
	inline glm::vec3 get_up() { return up; };
};

