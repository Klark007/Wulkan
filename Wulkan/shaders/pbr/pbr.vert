#version 450
#extension GL_EXT_buffer_reference : require
// Simple vertex shader

layout (location = 0) out vec3 outColor;

#include "../common.shader"

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	mat4 model;
	VertexBuffer vertex_buffer;
} pc;

void main() 
{	
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * pc.model * vec4(v.position, 1.0f);
	outColor = v.color.rgb;
}
