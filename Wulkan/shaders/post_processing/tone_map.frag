#version 450
// tone mapping fragment shader

#define REDEFINE_PUSH_CONSTANT // We want our special push constant definition
#include "../common.shader" 

layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
} pc;

layout(set = 1, binding = 0) uniform sampler2D frame;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(1-texture(frame, inUV).rgb, 1);
}