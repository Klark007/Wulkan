#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;

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
	vec2 offset;
} push_constant;

void main() 
{
	//const array of positions for the triangle
	/*
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f)
	);

	//const array of colors for the triangle
	const vec3 colors[3] = vec3[3](
		vec3(1.0f, 0.0f, 0.0f), //red
		vec3(0.0f, 1.0f, 0.0f), //green
		vec3(00.f, 0.0f, 1.0f)  //blue
	);
	*/

	//output the position of each vertex
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = vec4(v.position.xy + push_constant.offset, 0.0f , 1.0f);
	outColor = v.color.xyz;
}
