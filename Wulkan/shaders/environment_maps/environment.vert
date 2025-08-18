#version 450
#extension GL_EXT_buffer_reference : require
// environment map vertex shader

#include "../common.shader" 

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
} push_constant;

layout(location = 0) out vec3 outUVW;

void main() {
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];
	
	vec4 pos = vec4(v.position, 1.0);

	mat4 view_mod = mat4(mat3(ubo.view)); // remove translation
	gl_Position = ubo.proj * view_mod * pos;
	
	// tex coords correspond to direction from center of cube outwards
	outUVW = v.position;
}