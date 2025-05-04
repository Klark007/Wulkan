#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 pos;
layout (location = 2) out vec3 outQ;
layout (location = 3) out float outSigma;

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
	vec2 _sun_direction;
	vec4 _sun_color;
} ubo;
#define PI 3.1415926538

void main() 
{	
	Vertex v = push_constant.vertex_buffer.vertices[gl_VertexIndex];

	vec3 q = normalize(vec3(0.8,0.2,1));
	outQ = q;
	float sigma = 0.2;
	outSigma = sigma;

	vec3 p = v.position;
	pos = p;

	float C = 1 / (sqrt(2) * pow(PI, 2.0/3.0) * sigma);
	float G = C * exp(-0.5 * pow(acos(dot(pos,q)) / sigma, 2)); 

	gl_Position = ubo.proj * ubo.view * vec4(v.position * (G + 0.5), 1.0f);
	outColor = v.color.rgb;
}
