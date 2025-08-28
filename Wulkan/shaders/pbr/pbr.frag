#version 450
// PBR implementation

#include "../common.shader"
#include "shadow.shader"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;

layout (location = 0) out vec4 outColor;

layout(std430, set = 2, binding = 0) uniform PBRData {
	vec3 diffuse;
	uint configuration;
} pbr_uniforms;

void main() 
{
	// compute cascade index
	vec4 view_pos = ubo.virtual_view * vec4(inWorldPos, 1.0); 

    float cos_theata =  max(
        dot(
            inWorldNormal, 
            spherical_to_dir(directional_light_ubo.light_direction)
        ), 0.1
    );

    uint cascade_idx = cascade_count;
    for (uint i = 0; i < cascade_count; i++) {
        if (-view_pos.z < directional_light_ubo.cascade_splits[i]) {
            cascade_idx = i;
            break;
        }
    }

    float in_shadow = 1.0;
    switch (directional_light_ubo.shadow_mode) {
        case 0:
            in_shadow = 1.0;
            break;
        case 1:
            in_shadow = max(shadow(inWorldPos, cascade_idx), 0.1);
            break;
        case 2:
            in_shadow = max(soft_shadow(inWorldPos, cascade_idx), 0.1);
            break;
        default:
            in_shadow = 1.0;
            break;
    }

	outColor =  vec4(pbr_uniforms.diffuse, 1) * vec4(directional_light_ubo.light_color, 1) * cos_theata * in_shadow;
}