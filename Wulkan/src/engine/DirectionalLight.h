#pragma once

#include "common.h"
#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_CommandPool.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_Buffer.h"
#include "Texture.h"
#include "Camera.h"

// TODO NAMING
struct ShadowDepthOnlyUniformData {
	alignas(16) glm::mat4 view_proj;
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

	void del() override;
private:
	glm::vec2 dir;
	glm::vec3 col;
	float str;

	bool cast_shadows;
	const VKW_Device* device;

public: // TODO: REMOVE public
	Camera shadow_camera;

	std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT> graphics_pools;
	std::array<VKW_CommandBuffer, MAX_FRAMES_IN_FLIGHT> cmds;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> shadow_semaphores;	
	std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers;


	Texture depth_rt;
public:
	inline void set_direction(glm::vec3 direction);
	void set_color(glm::vec3 color) { col = color; };
	void set_intensity(float intensity) { str = intensity; };
	

	glm::vec4 get_color() const { return glm::vec4(col, 1) * str; };
	glm::vec2 get_direction() const { return dir; };
};

inline void DirectionalLight::set_direction(glm::vec3 direction)
{
	dir = dir_to_spherical(direction);

	shadow_camera.set_dir(-direction);
}