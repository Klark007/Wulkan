#ifndef TERRAIN_COMMON_INCLUDE
#define TERRAIN_COMMON_INCLUDE

#include "../common.shader"
#extension GL_EXT_buffer_reference : require

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
    mat4 model;
    VertexBuffer vertex_buffer;
    float tesselation_strength;
    float max_tesselation;
    float texture_eps;
    int visualization_mode;
    int cascade_idx;
} pc;

layout(set = 2, binding = 0) uniform sampler2D albedo;
layout(set = 2, binding = 1) uniform sampler2D normal_map;
layout(set = 2, binding = 2) uniform sampler2D height_map;
layout(set = 2, binding = 3) uniform sampler2D curvature;

bool is_culled(vec3 pos[4], vec2 uv[4], mat4 MVP, sampler2D hm) {
    // extract frustum planes in object space https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    
    vec4 left = vec4(0);
    for (int i = 0; i < 4; i++) {
        left[i] = MVP[i][3] + MVP[i][0];
    }
    
    vec4 right = vec4(0);
    for (int i = 0; i < 4; i++) {
        right[i] = MVP[i][3] - MVP[i][0];
    }

    vec4 bottom = vec4(0);
    for (int i = 0; i < 4; i++) {
        bottom[i] = MVP[i][3] + MVP[i][1];
    }

    vec4 top = vec4(0);
    for (int i = 0; i < 4; i++) {
        top[i] = MVP[i][3] - MVP[i][1];
    }

    vec4 near = vec4(0);
    for (int i = 0; i < 4; i++) {
        near[i] = MVP[i][3] + MVP[i][2];
    }

    vec4 far = vec4(0);
    for (int i = 0; i < 4; i++) {
        far[i] = MVP[i][3] - MVP[i][2];
    }
    
    vec4 planes[6] = vec4[6](left, right, bottom, top, near, far);


    // bounding box
    float eps = 0.01; // bias as texture is only sampled at the vertices of the patch
    vec3 min_pos = pos[0];
    min_pos.z = (texture(hm, uv[0]).r - eps);

    vec3 max_pos = pos[0];
    max_pos.z = (texture(hm, uv[0]).r + eps);
    
    for (int i=1; i < 4; i++) {
        vec3 pos = pos[i];
        pos.z = texture(hm, uv[i]).r;

        min_pos = min(min_pos, pos);
        max_pos = max(max_pos, pos);
    }

    for (int i = 0; i < 6; i++) {
        bool outside = true;

        outside = outside && dot(planes[i], vec4(min_pos.x, min_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, min_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, max_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, min_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, max_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, max_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, min_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, max_pos.y, max_pos.z, 1)) < 0;

        if (outside) {
            return true; // is culled
        }
    }

    return false;
}

#endif