#version 450
// Controlls how strong this quad will be tesselated

#include "../common.shader"
#include "terrain_common.shader"

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec4 inColor[];

layout(binding = 4) uniform sampler2D height_map;
layout(binding = 5) uniform sampler2D curvature;

layout (vertices = 4) out;

layout(location = 0) out vec2 outUV[4];
layout(location = 1) out vec3 outNormal[4];
layout(location = 2) out vec4 outColor[4];

vec4 debug_level(float str) {
    if (str < pc.max_tesselation / 4) {
        return vec4(0,1,0,1);
    } else if (str < pc.max_tesselation / 2) {
        return vec4(0,1,1,1);
    } else if (str < 3 * pc.max_tesselation / 4) {
        return vec4(0,0,1,1);
    } else {
        return vec4(1,0,0,1);
    }
}

vec4 compute_tesselation_level();

void main()
{
    if (gl_InvocationID == 0)
    {
        if (is_culled(
            vec3[4](inPos[0], inPos[1], inPos[2], inPos[3]), 
            vec2[4](inUV[0], inUV[1], inUV[2], inUV[3]), 
            ubo.proj * ubo.virtual_view * pc.model, 
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
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    
    switch (pc.visualization_mode) {
        case 0: // shading
            outColor[gl_InvocationID] = inColor[gl_InvocationID];
            break;
        case 1: // tesselation level
            outColor[gl_InvocationID] = debug_level(
                min(
                    compute_tesselation_level() * pc.tesselation_strength, 
                    pc.max_tesselation
                )[gl_InvocationID]
            );
            break;
        default: // Not implemented modes or modes where color is computed in the fragment shader (pink) 
            outColor[gl_InvocationID] = vec4(1,0,1,1);
            break;
    } 
}


vec4 project_point(vec4 p, vec2 uv) {
    p.z = texture(height_map, uv).r;
    vec4 proj_p = ubo.proj * ubo.virtual_view * pc.model * p;
    proj_p = proj_p / proj_p.w;

    return proj_p;
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

        float linear_depth = linearize_depth_01(project_point(vec4(inPos[i],1), inUV[i]).z, ubo.near_far_plane.x, ubo.near_far_plane.y);

        res[i] = abs_h / nr_samples * inout_bezier(1 - linear_depth);
    }

    return res;
}