#version 450
// Simple vertex shader

layout (location = 0) out vec3 outColor;

#include "../common.shader"

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * vec4(v.position, 1.0f);
	outColor = v.color.rgb;
}
