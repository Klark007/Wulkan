#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	float _uv_x;
	vec3 _normal;
	float _uv_y;
	vec4 _color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	VertexBuffer vertex_buffer;
} push_constant;

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 _inv_view;
    mat4 _virtual_view;
    mat4 proj;
    vec2 _near_far_plane;
	vec2 _sun_direction;
	vec4 _sun_color;
} ubo;

layout(location = 0) out vec3 outUVW;

void main() {
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];
	
	vec4 pos = vec4(v.position, 1.0);

	mat4 view_mod = mat4(mat3(ubo.view)); // remove translation
	gl_Position = ubo.proj * view_mod * pos;
	
	// tex coords correspond to direction from center of cube outwards
	outUVW = v.position;
}