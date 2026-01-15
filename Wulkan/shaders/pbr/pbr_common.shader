#ifndef PBR_COMMON_INCLUDE
#define PBR_COMMON_INCLUDE

layout(std430, set = 2, binding = 0) uniform PBRData {
	vec3 diffuse;
	float metallic;

	vec3 specular;
	float roughness;

	vec3 emission; // Not supported yet
	float eta;

	// TODO: add ambient
	uint configuration;
} pbr_uniforms;

layout(set = 2, binding = 1) uniform sampler2D diffuse_tex;

	// TODO: Could share cos^2 instead of just cos_theta
vec3 diffuse(vec3 albedo, float cos_theta_i, float cos_theta_o, float cos_theta_d) {
	float t1 = pow(1 - cos_theta_i, 5);
	float t2 = pow(1 - cos_theta_o, 5);

	float FD90 = 0.5 + 2 * pbr_uniforms.roughness * cos_theta_d * cos_theta_d;

	return albedo * (1 + (FD90 - 1)*t1) * (1 + (FD90 - 1)*t2);
}

float eval_D(float cos_theta_h) {
	// see B2 Eq 8
	float roughness = max(pbr_uniforms.roughness, 1e-5);
	float alpha = roughness * roughness;
	// TODO check about div by pi
	float temp = 1 + (alpha * alpha - 1) * cos_theta_h * cos_theta_h;
	return alpha * alpha * INV_PI / (temp * temp);
}

// based on https://schuttejoe.github.io/post/disneybsdf/
vec3 calculate_tint(vec3 base_color) {
	float luminance = dot(vec3(0.3, 0.6, 1.0), base_color);
    return (luminance > 0.0f) ? base_color * (1.0f / luminance) : vec3(1);
}

// fresnel approx
vec3 eval_F(vec3 albedo, float cos_theta_d) {
	float eta = pbr_uniforms.eta;

	float temp0 = (eta - 1) / (eta + 1);
	float F0 = temp0 * temp0;

	// TODO compare between calculate_tint and just albedo
	
	// this assumes specTint = 1 otherwise would need to 
	// t1 = F0 * mix(vec3(1), spec, specTint)
	// R0 = mix(t1, albedo, metallic)
	vec3 R0 = mix(F0 * calculate_tint(pbr_uniforms.specular), albedo, pbr_uniforms.metallic);

	float F_weight = pow(1 - cos_theta_d, 5);

	return mix(R0, vec3(1), F_weight); // R0 + (1-R0) * F_weight
}

// see specular G revisited; Masking-Shadowing Function in Microfacet-Based BRDFs (sect. 5.3)
float eval_G(float cos_theta) {
	if (cos_theta <= 0) {
		return 0;
	}

	float tan_theta = sqrt(1 - cos_theta * cos_theta) / cos_theta;

	// different remampping then for eval_D
	float temp = 0.5 + pbr_uniforms.roughness / 2;
	float alpha = temp * temp;

	float one_over_a = alpha * tan_theta;
	float lamba = (-1 + sqrt(1 + one_over_a * one_over_a)) / 2;
	return 1 / (1 + lamba);
}

vec3 specular(vec3 albedo, float cos_theta_i, float cos_theta_o, float cos_theta_d, float cos_theta_h) {	
	float D = eval_D(cos_theta_h);
	vec3 F = eval_F(albedo, cos_theta_d);

	float G = eval_G(cos_theta_i) * eval_G(cos_theta_o);

	return (D * F * G) / (4 * cos_theta_i * cos_theta_o);
}

vec4 pbr(vec3 w_i, vec3 w_o, vec3 n, vec3 light_color, vec2 uv, float in_shadow) {
	vec3 w_h = normalize(w_i + w_o);
	
	// angle between incoming/outcoming and normal
	float cos_theta_i = dot(w_i, n);
	float cos_theta_o = dot(w_o, n);

	uint use_diffuse_texture = pbr_uniforms.configuration & 1<<0;
	vec4 diffuse_col = texture(diffuse_tex, uv).rgba;

	float alpha = (1 - use_diffuse_texture) + use_diffuse_texture * diffuse_col.a; // for transparency only the alpha of the diffuse texture counts
	if (cos_theta_i <= 0 || cos_theta_o <= 0) {
		return vec4(0,0,0,alpha);
	}

	// angle between w_h and w_i (or w_o)
	float cos_theta_d = dot(w_i, w_h);
	float cos_theta_h = dot(w_h, n);

	float diffuse_weight = 1 - pbr_uniforms.metallic;

	vec3 albedo = (1 - use_diffuse_texture) *  pbr_uniforms.diffuse + use_diffuse_texture * diffuse_col.rgb;
	vec3 color = specular(albedo,  cos_theta_i, cos_theta_o, cos_theta_d, cos_theta_h);
	
	if (diffuse_weight > 0) {
		color += diffuse_weight * diffuse(albedo, cos_theta_i, cos_theta_o, cos_theta_d);
	}
	// + ambient * albedo
	
	return vec4(color * light_color * max(cos_theta_i * in_shadow, 0.05), alpha);
}

#endif