#include "Engine.h"

#include <GLFW/glfw3.h>

Engine::Engine(unsigned int res_x, unsigned int res_y, std::shared_ptr<Camera> camera)
	: res_x { res_x },
	  res_y { res_y }
{
	init_glfw();
	init_vulkan();


	camera_controller = CameraController(window, camera);
}

Engine::~Engine()
{
	surface.reset();
	instance.reset();

	glfwDestroyWindow(window);
	glfwTerminate();
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

// TODO: Query glfwGetRequiredInstanceExtensions() and add it to list of required extensions
void Engine::init_glfw()
{
	if (!glfwInit()) {
		throw SetupException("GLFW Initilization failed", __FILE__, __LINE__);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(res_x, res_y, "Vulkan", nullptr, nullptr);

	if (!window) {
		glfwTerminate();
		throw SetupException("GLFW Window creation failed", __FILE__, __LINE__);
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, glfw_window_resize_callback);

	// disable cursor so can use raw mouse movement
	if (!glfwRawMouseMotionSupported()) {
		throw std::runtime_error("Raw mouse motion not supported");
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, glfm_mouse_move_callback);
}

void Engine::init_vulkan()
{
	init_instance();
	create_surface();
}

void Engine::init_instance()
{
	instance = std::make_shared<VKW_Instance>("VK Study", get_required_instance_extensions(), std::vector<const char*>());
}

void Engine::create_surface()
{
	surface = std::make_shared<VKW_Surface>(window, instance);
}

std::vector<const char*> Engine::get_required_instance_extensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	return extensions;
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

inline void Engine::resize()
{
}

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y) {
	Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (engine) {
		engine->camera_controller.handle_mouse(pos_x, pos_y);
	}
	else {
		throw SetupException("GLFW Engine User pointer not set", __FILE__, __LINE__);
	}
}

void glfw_window_resize_callback(GLFWwindow* window, int width, int height) {
	Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (engine) {
		engine->resize_callback(width, height);
	}
	else {
		throw SetupException("GLFW Engine User pointer not set", __FILE__, __LINE__);
	}
}
