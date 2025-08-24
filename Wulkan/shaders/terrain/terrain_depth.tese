#version 450
#extension GL_EXT_scalar_block_layout : enable
// depth only version of terrain.tese

#include "terrain_common.shader"
#define SHADOW_UNIFORM_ONLY // Don't want the actual shadow functions
#include "../pbr/shadow.shader"

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];

void main()
{
    vec2 uv1 = mix(inUV[0], inUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(inUV[3], inUV[2], gl_TessCoord.x);
	vec2 outUV = mix(uv1, uv2, gl_TessCoord.y);

    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

    pos.z = texture(height_map, outUV).r;

    gl_Position = directional_light_ubo.proj_views[pc.cascade_idx] * pc.model * pos;
}

