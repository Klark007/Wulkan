#include "DirectionalLight.h"

void DirectionalLight::init(glm::vec3 direction, glm::vec3 color, float intensity)
{
	dir = dir_to_spherical(direction);
	col = color;
	str = intensity;

	cast_shadows = false;
}

void DirectionalLight::init(const VKW_Device* vkw_device, const std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT>& pools, glm::vec3 destination, glm::vec3 direction, float distance, uint32_t shadow_res_x, uint32_t shadow_res_y, float orthographic_height, float near_plane, float far_plane)
{
	device = vkw_device;
	graphics_pools = pools;
	cast_shadows = true;
	dest = destination;
	dist = distance;

	shadow_camera = Camera(dest + direction * dist, dest, shadow_res_x, shadow_res_y, glm::radians(45.0f), near_plane, far_plane);
	shadow_camera.set_orthographic_projection_height(orthographic_height);

	// TODO: Array texture for each cascade
	depth_rt.init(
		device,
		shadow_res_x,
		shadow_res_y,
		Texture::find_format(*device, Texture_Type::Tex_D),
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(),
		"Directional light shadow map"
	);

	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_CHECK_ET(vkCreateSemaphore(*device, &semaphore_create_info, nullptr, &shadow_semaphores.at(i)), SetupException, "Failed to create a semaphore for the directional shadow maps");
		device->name_object((uint64_t)shadow_semaphores.at(i), VK_OBJECT_TYPE_SEMAPHORE, "Shadow map semaphore");

		uniform_buffers.at(i).init(
			device,
			sizeof(ShadowDepthOnlyUniformData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sharing_exlusive(),
			true,
			"Shadow Uniform buffer"
		);
		uniform_buffers.at(i).map();


		cmds.at(i).init(device, &graphics_pools.at(i), false, "Shadow CMD");

	}
}

void DirectionalLight::del()
{
	if (cast_shadows) {
		depth_rt.del();

		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VK_DESTROY(shadow_semaphores.at(i), vkDestroySemaphore, *device, shadow_semaphores.at(i));
			uniform_buffers.at(i).del();
		}
	}
}
