#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};


layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
} push_constant;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec2 uvCoord;


void main() {
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];

    gl_Position = vec4(v.position, 1.0);
    outPos = vec4(v.position, 1.0);
    uvCoord = vec2(v.uv_x, v.uv_y);
}