#version 450
#extension GL_EXT_buffer_reference : require
// Simple vertex shader

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;

#include "../common.shader"
#define SHADOW_UNIFORM_ONLY // Don't want the actual shadow functions
#include "shadow.shader"

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	mat4 model;
	mat4 inv_model;
	VertexBuffer vertex_buffer;
	int cascade_idx;
} pc;

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = directional_light_ubo.proj_views[pc.cascade_idx] * pc.model * vec4(v.position, 1.0f);
}