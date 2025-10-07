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

enum ShadowMode {
	NoShadows,
	HardShadows,
	SoftShadows
};

// only used if shadows can be cast onto object
struct DirectionalLightUniform {
	alignas(16) glm::mat4 proj_view[MAX_CASCADE_COUNT];
	alignas(4) float split_planes[MAX_CASCADE_COUNT];
	alignas(8) glm::vec2 shadow_extends[MAX_CASCADE_COUNT]; // phyical extends of orthographic projections horizontaly and vertically
	alignas(16) glm::vec3 scaled_color;                     // already scaled by intensity
	alignas(4) float receiver_sample_region;                // how much the samples for if we are in shadow are distributed
	alignas(8) glm::vec2 direction;

	alignas(4) float occluder_sample_region;                // how much the samples for average occluder distance are distributed 
	alignas(4) int nr_shadow_receiver_samples;
	alignas(4) int nr_shadow_occluder_samples;
	alignas(4) int shadow_mode;							    // 0: no shadows, 1: hard shadows, 2: soft shadows
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
	
	void init_debug_lines(const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass);

	static VKW_DescriptorSetLayout create_shadow_descriptor_layout(const VKW_Device& device);

	void del() override;
private:
	const VKW_Device* device = nullptr;

	glm::vec2 dir;
	glm::vec3 col;
	float str;

	bool cast_shadows;
	glm::vec3 dest;
	float dist;

	float r_sample_reg;
	float o_sample_reg;
	int r_nr_samples;
	int o_nr_samples;

	ShadowMode shadow_mode;

	Camera shadow_camera;
	// weighs between uniform distributing cascade splits and distributing based on distance (more closer)
	float lambda;

	std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT> graphics_pools;
	std::array<VKW_CommandBuffer, MAX_FRAMES_IN_FLIGHT> cmds;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> shadow_semaphores; // signal when shadow depth pass is finished

	std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;

	Texture depth_rt;
	int res_x, res_y; // resolution of texture (same for all cascade layers)

	bool initialized_debug_lines;
	std::array<glm::vec4, 4> line_colors = {
		glm::vec4(1,0,0,1),
		glm::vec4(1,1,0,1),
		glm::vec4(0,1,0,1),
		glm::vec4(0,0,1,1)
	};

	std::array<Line, MAX_CASCADE_COUNT> splitted_camera_frustums;
	std::array<Frustum, MAX_CASCADE_COUNT> shadow_camera_frustums;
public:
	void set_uniforms(const Camera& camera, int nr_cascades, int current_frame);
	
	const VKW_CommandBuffer& begin_depth_pass(int current_frame);
	void end_depth_pass(int current_frame);

	void draw_debug_lines(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, int nr_current_cascades);
	Texture& get_texture() { return depth_rt; };
	const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> get_uniform_buffers() const { return uniform_buffers; };
	VkSemaphore get_shadow_pass_semaphore(int current_frame) const { return shadow_semaphores.at(current_frame); };

	inline void set_direction(glm::vec3 direction);
	void set_lambda(float l) { lambda = l; };
	void set_color(glm::vec3 color) { col = color; };
	void set_intensity(float intensity) { str = intensity; };
	inline void set_sample_info(float receiver_sample_region, float occluder_sample_region, int nr_shadow_receiver_samples, int nr_shadow_occluder_samples);
	void set_shadow_mode(ShadowMode m) { shadow_mode = m; };
};

inline void DirectionalLight::set_direction(glm::vec3 direction)
{
	dir = dir_to_spherical(direction);

	shadow_camera.set_pos(dest + direction * dist);
	shadow_camera.look_at(dest);
}

inline void DirectionalLight::set_sample_info(float receiver_sample_region, float occluder_sample_region, int nr_shadow_receiver_samples, int nr_shadow_occluder_samples)
{
	r_sample_reg = receiver_sample_region;
	o_sample_reg = occluder_sample_region;
	r_nr_samples = nr_shadow_receiver_samples;
	o_nr_samples = nr_shadow_occluder_samples;
}
