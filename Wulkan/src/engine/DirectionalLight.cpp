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

	depth_rt.init(
		device,
		shadow_res_x,
		shadow_res_y,
		Texture::find_format(*device, Texture_Type::Tex_D),
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(),
		"Directional light shadow map",
		0,
		MAX_CASCADE_COUNT
	);

	res_x = shadow_res_x;
	res_y = shadow_res_y;

	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_CHECK_ET(vkCreateSemaphore(*device, &semaphore_create_info, nullptr, &shadow_semaphores.at(i)), SetupException, "Failed to create a semaphore for the directional shadow maps");
		device->name_object((uint64_t)shadow_semaphores.at(i), VK_OBJECT_TYPE_SEMAPHORE, "Shadow map semaphore");

		uniform_buffers.at(i).init(
			device,
			sizeof(DirectionalLightUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sharing_exlusive(),
			true,
			"Shadow Uniform buffer"
		);
		uniform_buffers.at(i).map();


		cmds.at(i).init(device, &graphics_pools.at(i), false, "Shadow CMD");
	}
}

void DirectionalLight::init_debug_lines(const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, MaterialType<PushConstants, 1>& material_type, const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers)
{
	if (!cast_shadows) {
		throw SetupException("Tried to initialize debug lines of shadow casting for a directional light not casting shadows", __FILE__, __LINE__);
	}
	initialized_debug_lines = true;

	for (unsigned int i = 0; i < MAX_CASCADE_COUNT; i++) {
		Line& camera_frustums = splitted_camera_frustums.at(i);
		camera_frustums.init(
			*device,
			transfer_pool,
			descriptor_pool,
			material_type,
			// data is overwriten anyways (but size needs to be correct)
			{
				glm::vec3(0,0,0), glm::vec3(1, 0, 0),  glm::vec3(1, 1, 0),  glm::vec3(0, 1, 0),
				glm::vec3(0,0,1), glm::vec3(1, 0, 1),  glm::vec3(1, 1, 1),  glm::vec3(0, 1, 1)
			},
			{
				0, 1, 1, 2, 2, 3, 3, 0,
				0, 4, 1, 5, 2, 6, 3, 7,
				4, 5, 5, 6, 6, 7, 7, 4
			},
			(i < 4) ? line_colors.at(i) : glm::vec4(1, 0, 1, 1)
		);
		camera_frustums.set_descriptor_bindings(uniform_buffers);

		Frustum& shadow_frustum = shadow_camera_frustums.at(i);
		shadow_frustum.init(
			*device,
			transfer_pool,
			descriptor_pool,
			material_type,
			glm::mat4(1), // is overwritten anyways
			(i < 4) ? line_colors.at(i) : glm::vec4(1, 0, 1, 1)
		);
		shadow_frustum.set_descriptor_bindings(uniform_buffers);
	}
}

void DirectionalLight::set_uniforms(const Camera& camera, int nr_current_cascades, int current_frame)
{
	DirectionalLightUniform uniform{};

	float camera_near_plane = camera.get_near_plane();
	float camera_far_plane = camera.get_far_plane();

	float zero_one_splits[MAX_CASCADE_COUNT + 1] = {}; // split planes (including near plane) normalized to 0 to 1
	zero_one_splits[0] = 0;

	// compute distance of splits (formula from original paper)
	for (int i = 1; i <= nr_current_cascades; i++) {
		float ratio = ((float)i) / nr_current_cascades;
		uniform.split_planes[i - 1] = lambda * camera_near_plane * powf(camera_far_plane / camera_near_plane, ratio)
			+ (1 - lambda) * (camera_near_plane + ratio * (camera_far_plane - camera_near_plane));

		float d = uniform.split_planes[i - 1];
		// inverted the function to linearize depth as this is already linear atm.
		zero_one_splits[i] = (camera_near_plane * camera_far_plane / d - camera_far_plane) / (camera_near_plane - camera_far_plane);
	}

	// project the edges of the cameras frustum
	glm::mat4 camera_proj = camera.generate_projection_mat();
	camera_proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication
	glm::mat4 camera_view = camera.generate_virtual_view_mat();
	glm::mat4 inv_proj_view_camera = glm::inverse(camera_proj * camera_view);

	// into the shadow "cameras" space
	float shadow_near_plane = shadow_camera.get_near_plane();
	float shadow_far_plane = shadow_camera.get_far_plane();
	glm::mat4 P = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, shadow_near_plane, shadow_far_plane);
	P[1][1] *= -1;
	glm::mat4 shadow_view_mat = shadow_camera.generate_view_mat();
	glm::mat4 shadow_project_view = P * shadow_view_mat;

	for (int cascade_idx = 0; cascade_idx < nr_current_cascades; cascade_idx++) {
		// frustum points in clip space
		glm::vec3 frustum_points[8] = {
			glm::vec3(-1.0f,  1.0f, zero_one_splits[cascade_idx]),
			glm::vec3(1.0f,  1.0f,  zero_one_splits[cascade_idx]),
			glm::vec3(1.0f, -1.0f,  zero_one_splits[cascade_idx]),
			glm::vec3(-1.0f, -1.0f,  zero_one_splits[cascade_idx]),
			glm::vec3(-1.0f,  1.0f,  zero_one_splits[cascade_idx + 1]),
			glm::vec3(1.0f,  1.0f,   zero_one_splits[cascade_idx + 1]),
			glm::vec3(1.0f, -1.0f,   zero_one_splits[cascade_idx + 1]),
			glm::vec3(-1.0f, -1.0f,   zero_one_splits[cascade_idx + 1]),
		};

		// keep track of min/max over all points, used to compute actual shadow projection
		glm::vec3 min_v = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 max_v = glm::vec3(std::numeric_limits<float>::min());

		std::vector<glm::vec3> camera_frustum_points{};

		for (int i = 0; i < 8; i++) {
			glm::vec4 frustum_world_space = inv_proj_view_camera * glm::vec4(frustum_points[i], 1);
			frustum_world_space /= frustum_world_space.w;

			camera_frustum_points.push_back(glm::vec3(frustum_world_space));

			glm::vec4 frustum_shadow_clip = shadow_project_view * frustum_world_space;
			frustum_shadow_clip /= frustum_shadow_clip.w;

			min_v = glm::min(min_v, glm::vec3(frustum_shadow_clip));
			max_v = glm::max(max_v, glm::vec3(frustum_shadow_clip));
		}

		// round min/max to pixel steps
		// pixel size with current extends
		float step_x = (max_v.x - min_v.x) / res_x;
		float step_y = (max_v.y - min_v.y) / res_y;
		glm::vec2 step = glm::vec2(step_x, step_y);
		
		// min is rounded down towards a step
		glm::vec2 min_v_prime = glm::floor(glm::vec2(min_v) / step) * step;

		// max is rounded up a step
		glm::vec2 max_v_prime = glm::ceil(glm::vec2(max_v) / step) * step;

		
		// compute projection matrix
		glm::mat4 P_z = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, shadow_near_plane, max_v.z * shadow_far_plane);
		P_z[1][1] *= -1;
		
		glm::mat4 C = glm::mat4(1);
		float S_x = 2 / (max_v_prime.x - min_v_prime.x);
		float S_y = 2 / (max_v_prime.y - min_v_prime.y);
		C[0][0] = S_x;
		C[1][1] = S_y;
		C[3][0] = -0.5 * (max_v_prime.x + min_v_prime.x) * S_x;
		C[3][1] = -0.5 * (max_v_prime.y + min_v_prime.y) * S_y;
		

		glm::mat4 ortho_proj = C * P_z;

		uniform.shadow_extends[cascade_idx].x = 2 / ortho_proj[0][0];
		uniform.shadow_extends[cascade_idx].y = -2 / ortho_proj[1][1];

		uniform.proj_view[cascade_idx] = ortho_proj * shadow_view_mat;

		if (initialized_debug_lines) {
			splitted_camera_frustums.at(cascade_idx).update_vertices(camera_frustum_points);
			shadow_camera_frustums.at(cascade_idx).set_camera_matrix(uniform.proj_view[cascade_idx]);
		}
	}

	// set other uniform details
	uniform.direction = dir;
	uniform.scaled_color = glm::vec4(col * str, 1);
	uniform.receiver_sample_region = r_sample_reg;
	uniform.occluder_sample_region = o_sample_reg;
	uniform.nr_shadow_receiver_samples = r_nr_samples;
	uniform.nr_shadow_occluder_samples = o_nr_samples;
	uniform.shadow_mode = shadow_mode;

	memcpy(uniform_buffers.at(current_frame).get_mapped_address(), &uniform, sizeof(DirectionalLightUniform));
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

	if (initialized_debug_lines) {
		for (unsigned int i = 0; i < MAX_CASCADE_COUNT; i++) {
			splitted_camera_frustums.at(i).del();
			shadow_camera_frustums.at(i).del();
		}
	}
}

const VKW_CommandBuffer& DirectionalLight::begin_depth_pass(int current_frame)
{
	const VKW_CommandBuffer& shadow_cmd = cmds.at(current_frame);
	shadow_cmd.begin();

	Texture::transition_layout(shadow_cmd, depth_rt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	return shadow_cmd;
}

void DirectionalLight::end_depth_pass(int current_frame)
{
	const VKW_CommandBuffer& shadow_cmd = cmds.at(current_frame);
	Texture::transition_layout(shadow_cmd, depth_rt, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	shadow_cmd.submit(
		{ },
		{ },
		{ shadow_semaphores.at(current_frame) },
		VK_NULL_HANDLE
	);
}

void DirectionalLight::draw_debug_lines(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline, int nr_current_cascades)
{
	if (!initialized_debug_lines) {
		throw SetupException("Tried to draw debug lines without initalizing them", __FILE__, __LINE__);
	}

	for (int i = 0; i < nr_current_cascades; i++) {
		splitted_camera_frustums.at(i).draw(command_buffer, current_frame, pipeline);
		shadow_camera_frustums.at(i).draw(command_buffer, current_frame, pipeline);
	}
}
