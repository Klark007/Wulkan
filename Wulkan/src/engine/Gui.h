#pragma once

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_Instance.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_Queue.h"
#include "vk_wrap/VKW_DescriptorPool.h"
#include "vk_wrap/VKW_Swapchain.h"
#include "vk_wrap/VKW_CommandBuffer.h"

#include "Terrain.h"

#include "iostream"

struct GUI_Input {
	float camera_rotation_speed = 0.001f;
	float camera_movement_speed = 5.0f;

	float terrain_tesselation = 10.0f;
	float max_terrain_tesselation = 50.0f;
	float terrain_height_scale = 0.6f;
	float terrain_texture_eps = 0.33f;
	
	TerrainVisualizationMode terrain_vis_mode = TerrainVisualizationMode::Shaded;
};

class GUI : public VKW_Object
{
public:
	GUI() = default;
	void init(GLFWwindow* window, const VKW_Instance& instance, const VKW_Device& device, const VKW_Queue& graphics_queue, const VKW_DescriptorPool& descriptor_pool, const VKW_Swapchain* vkw_swapchain);
	void del() override;

	// draws directly into current swap chain image (in format VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	void draw(const VKW_CommandBuffer& cmd, uint32_t image_idx);
private:
	const VKW_Swapchain* swapchain;

	GUI_Input data;

	void draw_gui(const VKW_CommandBuffer& cmd);
public:
	inline const GUI_Input& get_input() const { return data; };
};

