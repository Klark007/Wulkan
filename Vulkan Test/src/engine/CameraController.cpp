#include "CameraController.h"

CameraController::CameraController(GLFWwindow* window, std::shared_ptr<Camera> camera)
	: active_camera { camera },
	  window { window } 
{
	glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
}

void CameraController::handle_keys()
{
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		pause_pressed = true;
	}
	else if (pause_pressed && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
		pause_pressed = false;
		if (!camera_paused) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		camera_paused = !camera_paused;
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		virtual_freeze_pressed = true;
	}
	else if (virtual_freeze_pressed && glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		virtual_freeze_pressed = false;

		virtual_camera_freeze = !virtual_camera_freeze;

		active_camera->set_virtual_camera_enabled(virtual_camera_freeze);
	}

	// check if cameras paused, otherwise process movement keys
	if (camera_paused)
		return;

	glm::vec2 delta = glm::vec2(0);

	if (glfwGetKey(window, GLFW_KEY_W))
		delta.x += 1;
	if (glfwGetKey(window, GLFW_KEY_S))
		delta.x -= 1;
	if (glfwGetKey(window, GLFW_KEY_D))
		delta.y += 1;
	if (glfwGetKey(window, GLFW_KEY_A))
		delta.y -= 1;

	delta = (delta != glm::vec2(0)) ? glm::normalize(delta) : delta;

	glm::vec3 pos = active_camera->get_pos();
	glm::vec3 dir = active_camera->get_dir();

	pos += delta.x * move_strength * dir * (float)delta_time;
	glm::vec3 side = glm::cross(dir, active_camera->get_up()); // can assume both normalized
	pos += delta.y * move_strength * side * (float)delta_time;

	active_camera->set_pos(pos);
}

void CameraController::handle_mouse(double pos_x, double pos_y)
{
	// dont rotate in paused
	if (camera_paused) {
		mouse_pos_x = pos_x;
		mouse_pos_y = pos_y;
		return;
	}

	double dx = pos_x - mouse_pos_x;
	double dy = pos_y - mouse_pos_y;

	if (dx != 0 || dy != 0) {
		// update yaw and pitch of camera
		active_camera->roll_yaw((float)(active_camera->points_up() ? dx : -dx) * rot_strength);
		active_camera->roll_pitch((float)(-dy * rot_strength));
	}

	mouse_pos_x = pos_x;
	mouse_pos_y = pos_y;
}

void CameraController::init_time()
{
	current_time = glfwGetTime();
	prev_time = current_time;
	delta_time = 1.0 / 60;
}

void CameraController::update_time()
{
	current_time = glfwGetTime();
	delta_time = current_time - prev_time;
	prev_time = current_time;
}


