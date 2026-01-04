#version 450
// tone mapping fragment shader
// see https://graphics-programming.org/blog/tone-mapping

#define REDEFINE_PUSH_CONSTANT // We want our special push constant definition
#include "../common.shader" 
#include "post_processing_common.shader"

layout(set = 1, binding = 0) uniform sampler2D frame;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

vec3 rheinhard(vec3 in_color) {
	float old_luminance = luminance(in_color);
	float new_luminance = old_luminance / (1 + old_luminance);
	return in_color * new_luminance / old_luminance;
}

vec3 ext_rheinhard(vec3 in_color) {
	float old_l = luminance(in_color);
	float new_l = (old_l * (1 + (old_l / (pc.l_white_point * pc.l_white_point)))) / (1 + old_l);
	return in_color * new_l / old_l;
}

void main() {
	vec3 color = texture(frame, inUV).rgb;

	if (isinf3(color)) {
		outColor = vec4(1);
		return;
	}

	switch (pc.tone_mapper_mode) {
		case 0: // None
			outColor = vec4(color, 1);
			break;
		case 1: // Rheinhard
			outColor = vec4(rheinhard(color), 1);
			break;
		case 2: // extended rheinhard
			outColor = vec4(ext_rheinhard(color), 1);
			break;
		default:
			outColor = vec4(1,0,1,1);
			break;
	}
}