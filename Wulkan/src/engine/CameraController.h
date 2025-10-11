#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Camera.h"

#include "Path.h"

class CameraController
{
public:
	// either call with camera or set_active_camera before calling handle_keys or handle_mouse
	CameraController() = default;
	CameraController(GLFWwindow* window, Camera* camera);

	void handle_keys();
	void handle_mouse(double pos_x, double pos_y);

	void init_time();
	void update_time();

	// Warning Only exports position and orientation but not intrinsics such as aspect ratio, fov, near and far planes
	void export_active_camera(const VKW_Path& path);
	void import_active_camera(const VKW_Path& path);

	inline double get_dt() const { return delta_time; };
private:
	GLFWwindow* window = nullptr;
	Camera* active_camera = nullptr;

	float rot_strength = 0.001f;
	float move_strength = 5.0f;

	// pause movement to interact with gui
	bool pause_pressed = false; // toggle camera can move
	bool camera_paused = false;

	// freeze virtual camera (important to test frustum culling)
	bool virtual_freeze_pressed = false; // toggle virtual camera freeze
	bool virtual_camera_freeze = false;

	// previous mouse position
	double mouse_pos_x = 0;
	double mouse_pos_y = 0;

	// for computing delta time
	double current_time = 0;
	double prev_time = 0;
	double delta_time = 1.0/60;
public:
	inline void set_active_camera(Camera* camera) { active_camera = camera; };
	inline void set_move_strength(float strength) { move_strength = strength; };
	inline void set_rotation_strength(float strength) { rot_strength = strength; };
};

