#pragma once

#include "Line.h"

// visualize a camera's frustum
class Frustum : public Line
{
public:
	Frustum() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, 1>& render_pass, glm::mat4 proj_view_mat, glm::vec4 color);
private:
	const glm::vec3 frustum_NDC[8] = {
		glm::vec3(-1.0f,  1.0f, 0.0f),
		glm::vec3(1.0f,   1.0f, 0.0f),
		glm::vec3(1.0f,  -1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f,  1.0f, 1.0f),
		glm::vec3(1.0f,   1.0f, 1.0f),
		glm::vec3(1.0f,  -1.0f, 1.0f),
		glm::vec3(-1.0f, -1.0f, 1.0f),
	};

	std::vector<glm::vec3> points;
public:
	void set_camera_matrix(glm::mat4 proj_view_mat);
};

