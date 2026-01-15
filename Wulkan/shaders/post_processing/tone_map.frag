#version 450
// tone mapping fragment shader
// see https://graphics-programming.org/blog/tone-mapping

#define REDEFINE_PUSH_CONSTANT // We want our special push constant definition
#include "../common.shader" 
#include "post_processing_common.shader"
#include "agx.shader"

layout(set = 1, binding = 0) uniform sampler2D frame;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

vec3 rheinhard(vec3 c);
vec3 ext_rheinhard(vec3 c);
vec3 uncharted_filmic(vec3 c);
vec3 aces_approx(vec3 v);

void main() {
	vec3 color = texture(frame, inUV).rgb;

	if (isinf3(color)) {
		outColor = vec4(1);
		return;
	}

	switch (pc.tone_mapper_mode) {
		case 0: // None
			outColor = vec4(color, 1);
			break;
		case 1: // Rheinhard
			outColor = vec4(rheinhard(color), 1);
			break;
		case 2: // extended rheinhard
			outColor = vec4(ext_rheinhard(color), 1);
			break;
		case 3: // Uncharted filmic
			outColor = vec4(uncharted_filmic(color), 1);
			break;
		case 4: // ACES
			outColor = vec4(aces_approx(color), 1);
			break;
		case 5: // AgX
			color = agx(color);
			color = agxLook(color);
			color = agxEotf(color);

			outColor = vec4(color, 1);
			break;
		default:
			outColor = vec4(1,0,1,1);
			break;
	}
}

vec3 rheinhard(vec3 c) {
	float old_l = luminance(c);
	float new_l = old_l / (1 + old_l);
	return c * new_l / old_l;
}

vec3 ext_rheinhard(vec3 c) {
	float old_l = luminance(c);
	float new_l = (old_l * (1 + (old_l / (pc.l_white_point * pc.l_white_point)))) / (1 + old_l);
	return c * new_l / old_l;
}

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// Look like bad paramters in the scene
vec3 uncharted_filmic(vec3 c)
{
    float exposure_bias = 2.0;
    vec3 curr = uncharted2_tonemap_partial(c * exposure_bias);

    vec3 W = vec3(11.2);
    vec3 white_scale = vec3(1.0) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

vec3 aces_approx(vec3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}
