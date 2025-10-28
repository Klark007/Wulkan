#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
// Simple vertex shader

layout (location = 0) out vec2 outUV;

#include "../common.shader"
#define SHADOW_UNIFORM_ONLY // Don't want the actual shadow functions
#include "shadow.shader"

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	vec3 local_pos = v.position;

	if (uint64_t(pc.instance_buffer) != 0) {
		Instance i = pc.instance_buffer.instances[gl_InstanceIndex];
		local_pos += i.position;
	}

	gl_Position = directional_light_ubo.proj_views[pc.cascade_idx] * pc.model * vec4(local_pos, 1.0f);
	outUV = vec2(v.uv_x, v.uv_y);
}