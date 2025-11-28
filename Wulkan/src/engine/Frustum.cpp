#include "common.h"
#include "Frustum.h"

void Frustum::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass , glm::mat4 proj_view_mat, glm::vec4 color)
{
	points.resize(8);
	
	std::vector<uint32_t> indices = {
		0, 1, 1, 2, 2, 3, 3, 0,
		0, 4, 1, 5, 2, 6, 3, 7,
		4, 5, 5, 6, 6, 7, 7, 4
	};

	// back side is darker
	std::vector<glm::vec4> colors = {
		color * 0.1f,
		color * 0.1f,
		color * 0.1f,
		color * 0.1f,
		color,
		color,
		color,
		color,
	};

	Line::init(device, transfer_pool, descriptor_pool, render_pass, points, indices, colors);
	set_camera_matrix(proj_view_mat);
}

void Frustum::set_camera_matrix(glm::mat4 proj_view_mat)
{
	glm::mat4 cam_inv_mat = glm::inverse(proj_view_mat);

	for (size_t i = 0; i < 8; i++) {
		glm::vec4 frustum_world_space = cam_inv_mat * glm::vec4(frustum_NDC[i], 1);
		frustum_world_space /= frustum_world_space.w;

		points.at(i) = glm::vec3(frustum_world_space);
	}

	update_vertices(points);
}
