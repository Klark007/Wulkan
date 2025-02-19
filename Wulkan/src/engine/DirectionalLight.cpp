#include "DirectionalLight.h"

void DirectionalLight::init(glm::vec3 direction, glm::vec3 color, float intensity)
{
	dir = dir_to_spherical(direction);
	col = color;
	str = intensity;

	cast_shadows = false;
}

void DirectionalLight::del()
{
	if (cast_shadows) {

	}
}
