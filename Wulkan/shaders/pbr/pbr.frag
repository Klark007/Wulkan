#version 450
// PBR implementation

#include "../common.shader"
#include "shadow.shader"

layout (location = 0) in vec3 inColor;
layout (location = 0) out vec4 outFragColor;

layout(std430, set = 2, binding = 0) uniform PBRData {
	vec3 diffuse;
	uint configuration;
} pbr_uniforms;

void main() 
{
	outFragColor = vec4(pbr_uniforms.diffuse, 1.0f);
}