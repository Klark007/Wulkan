#version 450
#extension GL_EXT_scalar_block_layout : enable
// depth only version of terrain.tesc

#include "terrain_common.shader"
#define SHADOW_UNIFORM_ONLY // Don't want the actual shadow functions
#include "../pbr/shadow.shader"

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];

layout (vertices = 4) out;

layout(location = 0) out vec2 outUV[4];

vec4 compute_tesselation_level();

void main()
{
    if (gl_InvocationID == 0)
    {
        if (is_culled(
            vec3[4](inPos[0], inPos[1], inPos[2], inPos[3]), 
            vec2[4](inUV[0], inUV[1], inUV[2], inUV[3]), 
            directional_light_ubo.proj_views[pc.cascade_idx] * pc.model, 
            height_map)) 
        {
            gl_TessLevelOuter[0] = 0;
            gl_TessLevelOuter[1] = 0;
            gl_TessLevelOuter[2] = 0;
            gl_TessLevelOuter[3] = 0;
                                   
            gl_TessLevelInner[0] = 0;
            gl_TessLevelInner[1] = 0;
        } else {
            vec4 str = compute_tesselation_level();

            gl_TessLevelOuter[0] = clamp(mix(str.w, str.x, 0.5) * pc.tesselation_strength, 1, pc.max_tesselation);
            gl_TessLevelOuter[1] = clamp(mix(str.x, str.y, 0.5) * pc.tesselation_strength, 1, pc.max_tesselation);
            gl_TessLevelOuter[2] = clamp(mix(str.y, str.z, 0.5) * pc.tesselation_strength, 1, pc.max_tesselation);
            gl_TessLevelOuter[3] = clamp(mix(str.z, str.w, 0.5) * pc.tesselation_strength, 1, pc.max_tesselation);

            gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
            gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);
        }
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;

    outUV[gl_InvocationID] = inUV[gl_InvocationID];
}

vec4 compute_tesselation_level() {
    // 1 pixel distance sample is a bad estimate of derivatives / curvature at a patches vertex
    vec2 epsilon = vec2(inUV[2] - inUV[0]) * pc.texture_eps;

    vec4 res = vec4(0);

    for (int i = 0; i < 4; i++) {

        int samples = 4;

        float abs_h = 0;
        int nr_samples = 0;
        // can be improved by non grid samples
        for (float dx = -epsilon.x; dx <= epsilon.x; dx += 2*epsilon.x / samples) {
            for (float dy = -epsilon.y; dy <= epsilon.y; dy += 2*epsilon.y / samples) {
                abs_h += texture(curvature, inUV[i] + vec2(dx, dy)).r;
                nr_samples += 1;
            }
        }

        
        res[i] = abs_h / nr_samples;
    }

    return res;
}