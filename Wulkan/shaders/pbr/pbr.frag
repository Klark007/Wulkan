#version 450
// PBR implementation

#include "../common.shader"
#include "shadow.shader"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;

layout (location = 0) out vec4 outColor;

layout(std430, set = 2, binding = 0) uniform PBRData {
	vec3 diffuse;
	uint configuration;
} pbr_uniforms;

void main() 
{
	float cos_theata =  dot(
        inWorldNormal, 
        spherical_to_dir(directional_light_ubo.light_direction)
    );

    float in_shadow = shadow(inWorldPos);
	outColor =  vec4(pbr_uniforms.diffuse, 1) * vec4(directional_light_ubo.light_color, 1) * max(cos_theata * in_shadow, 0.05);
}