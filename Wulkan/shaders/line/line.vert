#version 450
#extension GL_EXT_buffer_reference : require
// Simple vertex shader

layout (location = 0) out vec3 outColor;

struct Vertex {
	vec3 position;
	float _uv_x;
	vec3 _normal;
	float _uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	VertexBuffer vertex_buffer;
	mat4 model;
} push_constant;

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 _inv_view;
    mat4 _virtual_view;
    mat4 proj;
    vec2 _near_far_plane;
} ubo;

void main() 
{	
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * vec4(v.position, 1.0f);
	outColor = v.color.rgb;
}
