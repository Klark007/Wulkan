#pragma once


/* BIG TODO's:
 	Disable safety checks if compiled in Release mode
*/

#include "vk_wrap/VKW_Instance.h"
#include "vk_wrap/VKW_Surface.h"
#include "vk_wrap/VKW_Device.h"

#include "vk_wrap/VKW_Queue.h"
#include "vk_wrap/VKW_Swapchain.h"

#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"

#include "vk_wrap/VKW_Buffer.h"

#include "vk_wrap/VKW_Shader.h"
#include "vk_wrap/VKW_DescriptorPool.h"
#include "vk_wrap/VKW_DescriptorSet.h"
#include "vk_wrap/VKW_PushConstants.h"
#include "vk_wrap/VKW_Sampler.h"
#include "vk_wrap/VKW_GraphicsPipeline.h"

#include "DeletionQueue.h"
#include "Texture.h"
#include "Mesh.h"
#include "Terrain.h"
#include "EnvironmentMap.h"

#include "Gui.h"

#include "CameraController.h"

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);
void glfw_window_resize_callback(GLFWwindow* window, int width, int height);

struct CommandStructs {
	VKW_CommandPool graphics_command_pool;
	VKW_CommandPool transfer_command_pool;
	VKW_CommandBuffer graphics_command_buffer;
};

struct SyncStructs {
	// swapchain_semaphore signaled once an image is available to be rendered into
	VkSemaphore swapchain_semaphore;
	// render_semapohore is signaled once the rendering is done and the image is availabe to be presented
	VkSemaphore render_semaphore;
	// render fence is signaled once the image has finished rendering
	VkFence render_fence;
};

struct UniformStruct {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 inv_view;
	alignas(16) glm::mat4 virtual_view;
	alignas(16) glm::mat4 proj;
	alignas(8) glm::vec2 near_far_plane;
};

// std430
struct PushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
	alignas(16) glm::mat4 model;
};

class Engine
{
public:
	Engine() = default;
	void init(unsigned int res_x, unsigned int res_y);
	~Engine();
	void run();
private:
	struct GLFWwindow* window;
	unsigned int res_x, res_y;

	unsigned int current_frame;
	void update();
	void draw();
	void present();
	void late_update(); // executed after draw

	bool resize_window = false; // set to true by resize_callback(), will execute resize to avoid issues with resources 
	void resize();

	void init_glfw();
	void init_vulkan();

	void init_instance();
	void create_surface();
	void create_device();
	void create_queues();

	void init_shared_data();

	void create_texture_samplers();
	void create_uniform_buffers();
	void create_descriptor_sets();

	void init_data();


	void init_render_targets();

	void create_graphics_pipelines();
	void create_swapchain();
	void recreate_swapchain();
	void recreate_render_targets();
	void create_command_structs(); // creates command pools and buffers
	void create_sync_structs(); // create fences and semaphores

	bool aquire_image(); // aquires new image from swapchain

	void update_uniforms(); // updates uniform buffers (Pushconstant's are changed per object so not in this call)

	std::vector<const char*> get_required_instance_extensions();
	std::vector<const char*> get_required_device_extensions();
	Required_Device_Features get_required_device_features();

public: // TODO: remove public
	VKW_Instance instance;
	VKW_Surface surface;
	VKW_Device device;

	VKW_Queue graphics_queue;
	VKW_Queue present_queue;
	VKW_Queue transfer_queue;

	VKW_Swapchain swapchain;

	Texture color_render_target;
	Texture depth_render_target;

	VKW_GraphicsPipeline terrain_pipeline;
	VKW_GraphicsPipeline terrain_wireframe_pipeline;
	VKW_GraphicsPipeline environment_map_pipeline;

	// general sampler for texture (Linear sampling, repeat address mode)
	VKW_Sampler linear_texture_sampler;
	VKW_Sampler mirror_texture_sampler;

	// Terrain data
	SharedTerrainData shared_terrain_data;
	Terrain terrain;

	SharedEnvironmentData shared_environment_data;
	EnvironmentMap environment_map;

	// Input into shaders
	VKW_DescriptorPool imgui_descriptor_pool;

	VKW_DescriptorPool descriptor_pool;

	std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;


	std::array<CommandStructs, MAX_FRAMES_IN_FLIGHT> command_structs;
	std::array<SyncStructs, MAX_FRAMES_IN_FLIGHT> sync_structs;

	inline const VKW_CommandPool& get_current_graphics_pool() const;
	inline const VKW_CommandPool& get_current_transfer_pool() const;
	inline const VKW_CommandBuffer& get_current_command_buffer() const;
	inline VkSemaphore get_current_swapchain_semaphore() const;
	inline VkSemaphore get_current_render_semaphore() const;
	inline VkFence get_current_render_fence() const;

	unsigned int current_swapchain_image_idx;
	DeletionQueue cleanup_queue;

	CameraController camera_controller;
	GUI gui;
	GUI_Input gui_input;
	Camera camera;
private:

public:

	inline void resize_callback(unsigned int new_x, unsigned int new_y);
	struct GLFWwindow* get_window() const { return window; };
};

inline const VKW_CommandPool& Engine::get_current_graphics_pool() const
{
	return command_structs[current_frame].graphics_command_pool;
}

inline const VKW_CommandPool& Engine::get_current_transfer_pool() const
{
	return command_structs[current_frame].transfer_command_pool;
}

inline const VKW_CommandBuffer& Engine::get_current_command_buffer() const
{
	return command_structs[current_frame].graphics_command_buffer;
}

inline VkSemaphore Engine::get_current_swapchain_semaphore() const
{
	return sync_structs[current_frame].swapchain_semaphore;
}

inline VkSemaphore Engine::get_current_render_semaphore() const
{
	return sync_structs[current_frame].render_semaphore;
}

inline VkFence Engine::get_current_render_fence() const
{
	return sync_structs[current_frame].render_fence;
}