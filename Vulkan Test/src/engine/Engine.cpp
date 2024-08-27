#include "Engine.h"

Engine::Engine(GLFWwindow* window, std::shared_ptr<Camera> camera)
	: window { window },
	  camera_controller { window, camera }
{
}

void Engine::run()
{
	camera_controller.init_time();

	while (!glfwWindowShouldClose(window)) {
		update();

		draw();

		late_update();
	}
}

void Engine::update()
{
	glfwPollEvents();

	camera_controller.update_time();
	camera_controller.handle_keys();
}

void Engine::draw()
{
}

void Engine::late_update()
{
}
