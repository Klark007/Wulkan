#ifndef SHARED_COMMON_INCLUDE
#define SHARED_COMMON_INCLUDE

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_buffer_reference : require

#define M_PI         3.14159265358979323846f
#define INV_PI       0.31830988618379067154f

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

#ifndef REDEFINE_PUSH_CONSTANT
layout( push_constant ) uniform PushConstant
{	
	mat4 model;
	mat4 inv_model;
	VertexBuffer vertex_buffer;
	int cascade_idx;
} pc;
#endif

#extension GL_EXT_scalar_block_layout : enable
layout(std430, set = 0, binding = 0) uniform UniformData {
    mat4 view;
    mat4 inv_view;
    mat4 virtual_view;
    mat4 proj;
    vec2 near_far_plane;
} ubo;

float linearize_depth(float d, float near, float far)
{
    return near * far / (far + d * (near - far));
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float linearize_depth_01(float d, float near, float far) {
    return map(
        linearize_depth(d, near, far),
        near, far,
        0, 1
    );
}

float inout_bezier(float x) {
    return x * x * (3 - 2*x);
}

vec3 spherical_to_dir(vec2 sph) {
    float cos_theta = cos(sph.x);
    float sin_theta = sin(sph.x);

    float cos_phi = cos(sph.y);
    float sin_phi = sin(sph.y);

    return vec3(
        sin_theta * cos_phi,
        sin_theta * sin_phi,
        cos_theta
    );
}

#endif