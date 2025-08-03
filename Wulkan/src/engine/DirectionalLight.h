#pragma once

#include "common.h"
#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_Buffer.h"

#include "Texture.h"
#include "Camera.h"
#include "Frustum.h"

constexpr int MAX_CASCADE_COUNT = 4; // the default init in the shader cannot be lower than the max used

// TODO NAMING
struct ShadowDepthOnlyUniformData {
	alignas(64) glm::mat4 proj_view[MAX_CASCADE_COUNT];
	alignas(16) float split_planes[MAX_CASCADE_COUNT];
	alignas(16) float shadow_extends[2*MAX_CASCADE_COUNT];
};

class DirectionalLight : public VKW_Object
{
public:
	DirectionalLight() = default;

	// init for direction light if does not cast shadows
	void init(glm::vec3 direction, glm::vec3 color, float intensity);
	
	// needs to be called if Directional light should cast shadows, use explicit setters for color, intensity
	// sets camera's position to destination + direction * distance
	void init(const VKW_Device* vkw_device, const std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT>& graphics_pools, glm::vec3 destination, glm::vec3 direction, float distance, uint32_t shadow_res_x, uint32_t shadow_res_y, float orthographic_height,float near_plane, float far_plane);
	
	void init_debug_lines(const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedLineData* shared_line_data, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers);

	void del() override;
private:
	glm::vec2 dir;
	glm::vec3 col;
	float str;

	bool cast_shadows;
	const VKW_Device* device;

	glm::vec3 dest;
	float dist;

	bool initialized_debug_lines;
	std::array<glm::vec4, 4> line_colors = {
		glm::vec4(1,0,0,1),
		glm::vec4(1,1,0,1),
		glm::vec4(0,1,0,1),
		glm::vec4(0,0,1,1)
	};

	std::array<Line, MAX_CASCADE_COUNT> splitted_camera_frustums; // because we need projected points in code anyways
	std::array<Frustum, MAX_CASCADE_COUNT> shadow_camera_frustums;
	Camera shadow_camera;
	// weighs between uniform distributing cascade splits and distributing based on distance (more closer)
	float lambda;

	std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT> graphics_pools;
	std::array<VKW_CommandBuffer, MAX_FRAMES_IN_FLIGHT> cmds;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> shadow_semaphores;	
	std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;

	Texture depth_rt;
public:
	void set_uniforms(const Camera& camera, int nr_cascades, int current_frame);
	inline void set_direction(glm::vec3 direction);
	void set_color(glm::vec3 color) { col = color; };
	void set_intensity(float intensity) { str = intensity; };
	
	const VKW_CommandBuffer& begin_depth_pass(int current_frame);
	void end_depth_pass(int current_frame);

	void draw_debug_lines(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline, int nr_current_cascades);
	Texture& get_texture() { return depth_rt; };
	const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> get_uniform_buffers() const { return uniform_buffers; };
	VkSemaphore get_shadow_pass_semaphore(int current_frame) const { return shadow_semaphores.at(current_frame); };

	// returns color scaled by intensity
	glm::vec4 get_scaled_color() const { return glm::vec4(col, 1) * str; };
	glm::vec2 get_direction() const { return dir; };
};

inline void DirectionalLight::set_direction(glm::vec3 direction)
{
	dir = dir_to_spherical(direction);

	shadow_camera.set_pos(dest + direction * dist);
	shadow_camera.look_at(dest);
}