#version 450
// Simple vertex shader

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;

#include "../common.shader"

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * pc.model * vec4(v.position, 1.0f);

	outWorldPos = vec3(pc.model * vec4(v.position, 1.0f));
	outNormal = mat3(transpose(pc.inv_model)) * v.normal;
	outUV = vec2(v.uv_x, v.uv_y);
}
