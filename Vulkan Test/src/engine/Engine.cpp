#include "Engine.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <GLFW/glfw3.h>

#include <iostream>

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
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_DESTROY(sync_structs.at(i).swapchain_semaphore, vkDestroySemaphore, device->get_device(), sync_structs.at(i).swapchain_semaphore);
		VK_DESTROY(sync_structs.at(i).render_semaphore, vkDestroySemaphore, device->get_device(), sync_structs.at(i).render_semaphore);
		VK_DESTROY(sync_structs.at(i).render_fence, vkDestroyFence, device->get_device(), sync_structs.at(i).render_fence);
	}

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		command_structs.at(i).graphics_command_pool.reset();
		command_structs.at(i).transfer_command_pool.reset();
	}

	destroy_swapchain();

	device.reset();
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
	create_device();
	create_queues();

	create_swapchain();
	
	create_command_structs();
	create_sync_structs();
}

void Engine::init_instance()
{
	instance = std::make_shared<VKW_Instance>("VK Study", get_required_instance_extensions(), std::vector<const char*>());
}

void Engine::create_surface()
{
	surface = std::make_shared<VKW_Surface>(window, instance);
}

void Engine::create_device()
{
	Required_Device_Features required_features = get_required_device_features();

	device = std::make_shared<VKW_Device>(instance, surface, get_required_device_extensions(), required_features);;
	
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device->get_physical_device(), &deviceProperties);
	std::cout << "Selected GPU:" << deviceProperties.deviceName << std::endl;
	std::cout << "Tesselation limit:" << deviceProperties.limits.maxTessellationGenerationLevel << std::endl;
}

void Engine::create_queues()
{
	graphics_queue = std::make_shared<VKW_Queue>(device, vkb::QueueType::graphics);
	present_queue  = std::make_shared<VKW_Queue>(device, vkb::QueueType::present);
	transfer_queue = std::make_shared<VKW_Queue>(device, vkb::QueueType::transfer);
}

void Engine::create_swapchain()
{
	swapchain = std::make_shared<VKW_Swapchain>(window, device);
}

void Engine::recreate_swapchain()
{
  // TODO wait device idle
	destroy_swapchain();

	// TODO swap to recreating using previous swapchain
	create_swapchain();
}

void Engine::destroy_swapchain()
{
	// Destroys images, image views and framebuffers associated with the swapchain
	// TODO: reset images etc.

	swapchain.reset();
}

void Engine::create_command_structs()
{
	// Use 1 command pool with one buffer per thread and image in flight

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::shared_ptr<VKW_CommandPool> graphics_pool = std::make_shared<VKW_CommandPool>(device, graphics_queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		command_structs.at(i) = {
			graphics_pool,
			std::make_shared<VKW_CommandPool>(device, transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT),
			std::make_shared<VKW_CommandBuffer>(device, graphics_pool)
		};
	}
}

void Engine::create_sync_structs()
{
	// TODO: look into Semaphore/Fence pool
	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start in signaled state

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		sync_structs.at(i) = {};
		VK_CHECK_E(vkCreateSemaphore(device->get_device(), &semaphore_create_info, nullptr, &sync_structs.at(i).swapchain_semaphore), SetupException);
		VK_CHECK_E(vkCreateSemaphore(device->get_device(), &semaphore_create_info, nullptr, &sync_structs.at(i).render_semaphore), SetupException);
		VK_CHECK_E(vkCreateFence(device->get_device(), &fence_create_info, nullptr, &sync_structs.at(i).render_fence), SetupException);
	}
}

bool Engine::aquire_image()
{
	VK_CHECK_E(vkWaitForFences(device->get_device(), 1, &sync_structs.at(current_frame).render_fence, VK_TRUE, UINT64_MAX), RuntimeException);
	
	VkResult aquire_image_result = vkAcquireNextImageKHR(
		device->get_device(), 
		swapchain->get_swapchain(),
		UINT64_MAX, 
		sync_structs.at(current_frame).swapchain_semaphore, 
		VK_NULL_HANDLE, 
		&current_swapchain_image_idx
	);
	if (aquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
		std::cout << aquire_image_result << "," << resize_window << std::endl;
	}
	else if (aquire_image_result != VK_SUCCESS && aquire_image_result != VK_SUBOPTIMAL_KHR) {
		throw RuntimeException("Failed to aquire image from swapchain: " + std::string(string_VkResult(aquire_image_result)), __FILE__, __LINE__);
	}
	
	
	// we have successfully aquired image, can reset fence
	VK_CHECK_E(vkResetFences(device->get_device(), 1, &sync_structs.at(current_frame).render_fence), RuntimeException);
	return false;
}

std::vector<const char*> Engine::get_required_instance_extensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	return extensions;
}

std::vector<const char*> Engine::get_required_device_extensions()
{
	// VK_KHR_SWAPCHAIN_EXTENSION_NAME is added automatically
	std::vector<const char*> extensions{};
	return extensions;
}

Required_Device_Features Engine::get_required_device_features()
{
	Required_Device_Features features{};
	// 1.0 features
	features.rf.samplerAnisotropy = true;
	features.rf.fillModeNonSolid = true;
	features.rf.tessellationShader = true;
	// 1.2 features
	features.rf12.bufferDeviceAddress = true;
	features.rf12.descriptorIndexing = true;
	// 1.3 features
	features.rf13.dynamicRendering = true;
	features.rf13.synchronization2 = true;

	return features;
}

void Engine::update()
{
	glfwPollEvents();

	camera_controller.update_time();
	camera_controller.handle_keys();
}

void Engine::draw()
{
	if (!aquire_image()) {
		return;
	}
}

void Engine::late_update()
{
	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

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
