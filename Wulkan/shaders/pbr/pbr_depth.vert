#version 450
// Simple vertex shader

layout (location = 0) out vec2 outUV;


#include "../common.shader"
#define SHADOW_UNIFORM_ONLY // Don't want the actual shadow functions
#include "shadow.shader"

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = directional_light_ubo.proj_views[pc.cascade_idx] * pc.model * vec4(v.position, 1.0f);
	outUV = vec2(v.uv_x, v.uv_y);
}