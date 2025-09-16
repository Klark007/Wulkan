#pragma once

#include "imgui.h"

#include <mutex>

#include "CameraController.h"
#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_Instance.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_Queue.h"
#include "vk_wrap/VKW_DescriptorPool.h"
#include "vk_wrap/VKW_Swapchain.h"
#include "vk_wrap/VKW_CommandBuffer.h"

#include "Terrain.h"
#include "DirectionalLight.h"

struct GUI_Input {
	float camera_rotation_speed = 0.001f;
	float camera_movement_speed = 25.0f;

	// Terrain
	float terrain_tesselation = 0.1f;
	float max_terrain_tesselation = 16.0f;
	float terrain_height_scale = 0.8f;
	float terrain_texture_eps = 1.0f;
	bool terrain_wireframe_mode = false;
	TerrainVisualizationMode terrain_vis_mode = TerrainVisualizationMode::Shaded;

	// Light / Shadows
	glm::vec3 sun_direction = glm::vec3(0, 0.8, 0.5);
	glm::vec3 sun_color = glm::vec3(0.8, 0.8, 1.0);
	float sun_intensity = 1.0f;
	float depth_bias = 40000.0f;
	float slope_depth_bias = 0.1f;
	int nr_shadow_cascades = 3;
	bool shadow_draw_debug_frustums = false;
	
	float receiver_sample_region = 64.0f; 
	float occluder_sample_region = 64.0f;
	int nr_shadow_receiver_samples = 18;
	int nr_shadow_occluder_samples = 8;

	ShadowMode shadow_mode = ShadowMode::SoftShadows;
};

class GUI : public VKW_Object
{
public:
	GUI() = default;
	void init(GLFWwindow* window, const VKW_Instance& instance, const VKW_Device& device, const VKW_Queue& graphics_queue, const VKW_DescriptorPool& descriptor_pool, const VKW_Swapchain* vkw_swapchain, CameraController* camera_controller, std::recursive_mutex* camera_recursive_mutex);
	void del() override;

	// draws directly into current swap chain image (in format VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	void draw(const VKW_CommandBuffer& cmd, uint32_t image_idx);
private:
	const VKW_Swapchain* m_swapchain;
	CameraController* m_camera_controller;
	std::recursive_mutex* m_camera_recursive_mutex;

	GUI_Input m_data;

	void draw_gui(const VKW_CommandBuffer& cmd);
public:
	inline const GUI_Input& get_input() const { return m_data; };
};

