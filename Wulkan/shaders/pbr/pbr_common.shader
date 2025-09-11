#ifndef PBR_COMMON_INCLUDE
#define PBR_COMMON_INCLUDE

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

vec3 pbr(vec3 w_i, vec3 w_o, vec3 n, vec3 light_color, float in_shadow) {
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

	return color * light_color * max(cos_theta_i * in_shadow, 0.05);
}

#endif