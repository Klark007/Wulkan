#pragma once


/* BIG TODO's:
 
	Implicit casts from VKW objects to their vulkan internals
	Disable safety checks if compiled in Release mode
*/

// Consider adding implicit conversions from Wrapped object to their vulkan base struct
#include "vk_wrap/VKW_Instance.h"
#include "vk_wrap/VKW_Surface.h"
#include "vk_wrap/VKW_Device.h"

#include "vk_wrap/VKW_Queue.h"
#include "vk_wrap/VKW_Swapchain.h"

#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"

#include "vk_wrap/VKW_Buffer.h"

#include "Texture.h"

#include "CameraController.h"

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);
void glfw_window_resize_callback(GLFWwindow* window, int width, int height);

constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

struct CommandStructs {
	std::shared_ptr<VKW_CommandPool> graphics_command_pool;
	std::shared_ptr<VKW_CommandPool> transfer_command_pool;
	std::shared_ptr<VKW_CommandBuffer> graphics_command_buffer;
};

struct SyncStructs {
	VkSemaphore swapchain_semaphore, render_semaphore;
	VkFence render_fence;
};

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
	unsigned int current_frame;
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
	void create_device();
	void create_queues();
public: // TODO: remove public
  void create_swapchain();
	void recreate_swapchain();
	void destroy_swapchain();
private:
	void create_command_structs(); // creates command pools and buffers
	void create_sync_structs(); // create fences and semaphores
	bool aquire_image(); // aquires new image from swapchain, can fail if swapchain is out of date

	std::vector<const char*> get_required_instance_extensions();
	std::vector<const char*> get_required_device_extensions();
	Required_Device_Features get_required_device_features();

public: // TODO: remove public
	std::shared_ptr<VKW_Instance> instance;
	std::shared_ptr<VKW_Surface> surface;
	std::shared_ptr<VKW_Device> device;

	std::shared_ptr<VKW_Queue> graphics_queue;
	std::shared_ptr<VKW_Queue> present_queue;
	std::shared_ptr<VKW_Queue> transfer_queue;

	std::shared_ptr<VKW_Swapchain> swapchain;

	std::array<CommandStructs, MAX_FRAMES_IN_FLIGHT> command_structs;
	std::array<SyncStructs, MAX_FRAMES_IN_FLIGHT> sync_structs;

	unsigned int current_swapchain_image_idx;
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