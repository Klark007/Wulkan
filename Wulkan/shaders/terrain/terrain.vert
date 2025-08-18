#version 450
#extension GL_EXT_buffer_reference : require
// pass vertices without projecting them

#include "../common.shader"

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	layout(offset = 96) VertexBuffer vertex_buffer;
} push_constant;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outColor;

void main() {
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];
	
	gl_Position = vec4(v.position, 1.0);

	outPos = v.position;
	outUV = vec2(v.uv_x, v.uv_y);
	outNormal = v.normal;
	outColor = v.color;
}