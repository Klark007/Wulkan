#version 450
// PBR implementation

#include "../common.shader"
#include "shadow.shader"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;

layout (location = 0) out vec4 outColor;

layout(std430, set = 2, binding = 0) uniform PBRData {
	vec3 diffuse;
	float metallic;

	vec3 specular;
	float roughness;

	vec3 emission;
	uint configuration;
} pbr_uniforms;

vec3 diffuse(float cos_theta_i, float cos_theta_o, float cos_theta_d) {
	float t1 = pow(1 - cos_theta_i, 5);
	float t2 = pow(1 - cos_theta_o, 5);

	vec3 lambert = pbr_uniforms.diffuse;
	float FD90 = 0.5 + 2 * pbr_uniforms.roughness * cos_theta_d * cos_theta_d;

	return lambert * (1 + (FD90 - 1)*t1) * (1 + (FD90 - 1)*t2);
}

void main() 
{
	vec3 w_i = normalize(vec3(0,0,100) - inWorldPos); // spherical_to_dir(directional_light_ubo.light_direction); // incoming / light dir
	vec3 n   = inWorldNormal; // reflect (halfway vector)
	
	vec3 camera_pos = vec3(ubo.inv_view[3][0], ubo.inv_view[3][1], ubo.inv_view[3][2]);
	vec3 w_o = normalize(camera_pos - inWorldPos); // view direction
	vec3 w_h = normalize(w_i + w_o);

	// angle between incoming/outcoming and normal
	float cos_theta_i = dot(w_i, n);
	float cos_theta_o = dot(w_o, n);
	// angle between w_h and w_i (or w_o)
	float cos_theta_d = dot(w_o, n);

	float diffuse_weight = 1 - pbr_uniforms.metallic;

	vec3 color = vec3(0);

	if (diffuse_weight > 0) {
		color += diffuse_weight * diffuse(cos_theta_i, cos_theta_o, cos_theta_d);
	}

    float in_shadow = shadow(inWorldPos);
	outColor =  vec4(color, 1) * vec4(directional_light_ubo.light_color, 1) * max(cos_theta_i * in_shadow, 0.05);
}