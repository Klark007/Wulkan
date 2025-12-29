#version 450
// tone mapping vertex shader

#define REDEFINE_PUSH_CONSTANT // We want our special push constant definition
#include "../common.shader" 

layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
} pc;

layout(location = 0) out vec2 outUV;

void main() {
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];
	
	vec2 d_pos = vec2(v.position.x*2 - 1, v.position.y*2 - 1); // NDC between -1 and 1
	gl_Position = vec4(d_pos.x, d_pos.y, 0.0, 1.0); 

	outUV = vec2(v.uv_x, v.uv_y);
}