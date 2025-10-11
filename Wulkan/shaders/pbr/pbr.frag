#version 450
// PBR implementation
#extension GL_EXT_demote_to_helper_invocation : require // to use discard

#include "../common.shader"
#include "shadow.shader"
#include "pbr_common.shader"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 w_i = spherical_to_dir(directional_light_ubo.light_direction); // incoming / light dir
	vec3 n   = normalize(inWorldNormal);
	
	vec3 camera_pos = vec3(ubo.inv_view[3][0], ubo.inv_view[3][1], ubo.inv_view[3][2]);
	vec3 w_o = normalize(camera_pos - inWorldPos); // view direction

    float in_shadow = shadow(inWorldPos);
	
	vec4 pbr_res = pbr(w_i, w_o, n, directional_light_ubo.light_color, inUV, in_shadow);

	if (pbr_res.a == 0) {
		discard;
	}
	outColor = pbr_res;
}