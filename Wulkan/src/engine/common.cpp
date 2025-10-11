#include "common.h"

glm::vec2 dir_to_spherical(const glm::vec3& dir)
{
	glm::vec2 res {
		acos(dir.z),
		atan2(dir.y, dir.x)
	};

	if (res.y < 0)
		res.y += 2.0f * static_cast<float>(M_PI);

	return res;
}
