#pragma once


/* BIG TODO's:
 	Disable safety checks if compiled in Release mode?
*/

#include "common.h"

#include <thread>
#include <atomic>
#include <mutex>

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
#include "Line.h"
#include "Frustum.h"
#include "DirectionalLight.h"
#include "ObjMesh.h"
#include "InstancedLODShape.h"

#include "Gui.h"

#include "CameraController.h"

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);

struct CommandStructs {
	VKW_CommandPool graphics_command_pool;
	VKW_CommandPool transfer_command_pool;
	VKW_CommandBuffer graphics_command_buffer;
	TracyVkCtx graphics_queue_tracy_context = nullptr;
};

struct SyncStructs {
	// swapchain_semaphore signaled once an image is available to be rendered into
	VkSemaphore swapchain_semaphore;
	// render_semapohore is signaled once the rendering is done and the image is availabe to be presented
	VkSemaphore render_semaphore;
	// render fence is signaled once the image has finished rendering
	VkFence render_fence;
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

	// Note on multithreading
	// Thus far we only seperate (glfw) IO from rendering (in seperate thread)
	// We don't force that when one finshes a frame that it needs to wait for the other one to finish as well
	// IO can have multiple frames or also only update every 2nd frame (i.e. if windows has many events)
	// See: https://stackoverflow.com/questions/32255136/glfw-pollevents-really-really-slow
	void render_thread_func();
	std::thread render_thread;
	std::atomic_bool should_window_close = false;

	void update();
	void draw();
	void present();
	void late_update(); // executed after draw

	bool resize_window = false; // will execute resize to avoid issues with resources 

	void init_logger();
	void init_glfw();
	void init_vulkan();

	void init_instance();
	void create_surface();
	void create_device();
	void create_queues();

	void create_texture_samplers();
	void create_uniform_buffers();
	void create_descriptor_set_pool();

	void init_descriptor_set_layouts();
	void init_data();
	void init_descriptor_sets();

	void init_render_targets();

	void create_render_passes();
	void create_swapchain();
	void recreate_swapchain();
	void recreate_render_targets(); // resizes textures that are being rendered into and correlate with window size
	void create_command_structs(); // creates command pools and buffers
	void create_sync_structs(); // create fences and semaphores

	bool aquire_image(); // aquires new image from swapchain

	void update_uniforms(); // updates uniform buffers (Pushconstant's are changed per object so not in this call)

	inline std::vector<const char*> get_required_instance_extensions();
	inline std::vector<const char*> get_required_device_extensions();
	Required_Device_Features get_required_device_features();

	VKW_Instance instance;
	VKW_Surface surface;
	VKW_Device device;

	VKW_Queue graphics_queue;
	VKW_Queue present_queue;
	VKW_Queue transfer_queue;

	VKW_Swapchain swapchain;

	const VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_4_BIT; //VK_SAMPLE_COUNT_4_BIT;
	const bool use_msaa = sample_count != VK_SAMPLE_COUNT_1_BIT;
	Texture color_render_target;
	Texture color_resolve_target; // we can't resolve into swapchain as we have a different format
	Texture depth_render_target;


	// mostly for debugging reasons
	std::array<RenderPass<TerrainPushConstants, 3>, MAX_CASCADE_COUNT> terrain_render_passes;
	RenderPass<TerrainPushConstants, 3>  terrain_depth_render_pass;
	std::array<RenderPass<TerrainPushConstants, 3>, MAX_CASCADE_COUNT> terrain_wireframe_render_passes;

	RenderPass<EnvironmentMapPushConstants, 2> environment_render_pass;

	RenderPass<PushConstants, 1> line_render_pass;

	RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> pbr_render_pass;
	RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> pbr_render_double_sided_pass;
	RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT> pbr_depth_pass;

	// general sampler for texture (Linear sampling, repeat address mode)
	VKW_Sampler nearest_texture_sampler;
	VKW_Sampler linear_texture_sampler;
	VKW_Sampler mirror_texture_sampler;
	VKW_Sampler shadow_map_gather_sampler;

	// Input into shaders
	VKW_DescriptorPool imgui_descriptor_pool;

	VKW_DescriptorPool descriptor_pool;
	VKW_DynamicDescriptorPool dyn_descriptor_pool;

	std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;

	std::array<CommandStructs, MAX_FRAMES_IN_FLIGHT> command_structs;
	std::array<SyncStructs, MAX_FRAMES_IN_FLIGHT> sync_structs;

	// Descriptor set layouts
	VKW_DescriptorSetLayout view_desc_set_layout;
	VKW_DescriptorSetLayout shadow_desc_set_layout;
	VKW_DescriptorSetLayout terrain_desc_set_layout;
	VKW_DescriptorSetLayout environment_desc_set_layout;
	VKW_DescriptorSetLayout pbr_desc_set_layout;

	// shared descriptor sets
	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> view_descriptor_sets;
	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> shadow_descriptor_sets;

	// Terrain data
	Terrain terrain;

	// Environment map
	EnvironmentMap environment_map;

	// Directional light
	DirectionalLight directional_light;

	Texture texture_not_found;
	std::array<ObjMesh, 4> meshes;
	InstancedLODShape<ObjMesh> lod_mesh;

	inline const VKW_CommandPool& get_current_graphics_pool() const;
	inline const VKW_CommandPool& get_current_transfer_pool() const;
	inline const VKW_CommandBuffer& get_current_command_buffer() const;
	inline const TracyVkCtx& get_current_tracy_context() const;
	inline VkSemaphore get_current_swapchain_semaphore() const;
	inline VkSemaphore get_current_render_semaphore() const;
	inline VkFence get_current_render_fence() const;

	unsigned int current_swapchain_image_idx;
	DeletionQueue cleanup_queue;

	std::recursive_mutex glfw_input_mutex; // needs to be locked to read/write to Camera and Camera Controller
	CameraController camera_controller;
	Camera camera;

	GUI gui;
	GUI_Input gui_input;
public:
	std::recursive_mutex& get_glfw_input_recursive_mutex() { return glfw_input_mutex; };
	CameraController& get_camera_controller() { return camera_controller; };

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

inline const TracyVkCtx& Engine::get_current_tracy_context() const
{
	return command_structs[current_frame].graphics_queue_tracy_context;
}

inline VkSemaphore Engine::get_current_swapchain_semaphore() const
{
	return sync_structs[current_frame].swapchain_semaphore;
}

inline VkSemaphore Engine::get_current_render_semaphore() const
{
	return sync_structs[current_swapchain_image_idx % MAX_FRAMES_IN_FLIGHT].render_semaphore;
}

inline VkFence Engine::get_current_render_fence() const
{
	return sync_structs[current_frame].render_fence;
}