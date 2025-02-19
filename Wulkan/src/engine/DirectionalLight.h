#pragma once

#include "common.h"
#include "vk_wrap/VKW_Object.h"

// used for shader
struct DirectionalData {
	alignas(16) glm::vec4 color;
	alignas(8) glm::vec2 direction;
};

class DirectionalLight : public VKW_Object
{
public:
	DirectionalLight() = default;

	// init for direction light if does not cast shadows
	void init(glm::vec3 direction, glm::vec3 color, float intensity);
	
	// needs to be called if Directional light should cast shadows, use explicit setters for direction, color, intensity
	// void init();
	
	void del() override;
private:
	glm::vec2 dir;
	glm::vec3 col;
	float str;

	bool cast_shadows;
public:
	void set_direction(glm::vec3 direction) { dir = dir_to_spherical(direction); };
	void set_intensity(glm::vec3 color) { col = color; };
	void set_color(float intensity) { str = intensity; };

	// get data to be passed to shader's
	inline DirectionalData get_shader_data() const;
};

inline DirectionalData DirectionalLight::get_shader_data() const
{
	return {
		glm::vec4(col, 1) * str,
		dir
	};
}