#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
// Simple vertex shader

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outColor;


#include "../common.shader"
#include "pbr_common.shader"

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	vec3 local_pos = v.position;

	if (uint64_t(pc.instance_buffer) != 0) {
		Instance i = pc.instance_buffer.instances[gl_InstanceIndex];
		local_pos += i.position;
	}

	gl_Position = ubo.proj * ubo.view * pc.model * vec4(local_pos, 1.0f);

	outWorldPos = vec3(pc.model * vec4(local_pos, 1.0f));
	outNormal = mat3(transpose(pc.inv_model)) * v.normal;
	outUV = vec2(v.uv_x, v.uv_y);

	uint visualization_mode = pbr_uniforms.configuration >> 16;
	switch (visualization_mode) {
		case 4: // visualize instance
			outColor = vec3(gl_InstanceIndex / 128.0, 0, 0);
			break;
		default:
			break;
	}
}
