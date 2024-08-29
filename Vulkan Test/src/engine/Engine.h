#pragma once

#include "vk_wrap/VKW_Instance.h"
#include "vk_wrap/VKW_Surface.h"

#include "CameraController.h"

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);
void glfw_window_resize_callback(GLFWwindow* window, int width, int height);

class Engine
{
public:
	Engine(unsigned int res_x, unsigned int res_y, std::shared_ptr<Camera> camera);
	~Engine();
	void run();
private:
	struct GLFWwindow* window;
	unsigned int res_x, res_y;

public:  // TODO: remove public
	void update();
	void draw();
	void late_update(); // executed after draw
private: // TODO: remove public

public: // TODO: remove public
	bool resize_window = false; // set to true by resize_callback(), will execute resize to avoid issues with resources 
private: // TODO: remove public
	void resize();

	void init_glfw();
	void init_vulkan();
	void init_instance();
	void create_surface();

	std::vector<const char*> get_required_instance_extensions();

public: // TODO: remove public
	std::shared_ptr<VKW_Instance> instance;
	std::shared_ptr<VKW_Surface> surface;
private:



public:
	CameraController camera_controller;

	inline void resize_callback(unsigned int new_x, unsigned int new_y);
	struct GLFWwindow* get_window() const { return window; };
};

// TODO: check if resize_callback could call resize directly
inline void Engine::resize_callback(unsigned int new_x, unsigned int new_y)
{
	res_x = new_x;
	res_y = new_y;
	resize_window = true;
}