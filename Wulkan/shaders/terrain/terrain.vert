#version 450
// pass vertices without projecting them

#include "terrain_common.shader"

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outColor;

void main() {
	Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];
	
	gl_Position = vec4(v.position, 1.0);

	outPos = v.position;
	outUV = vec2(v.uv_x, v.uv_y);
	outNormal = v.normal;
	outColor = v.color;
}