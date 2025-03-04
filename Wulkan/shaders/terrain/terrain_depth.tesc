#version 450

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec4 inColor[];

layout(binding = 0) uniform UniformData {
    mat4 _view;
    mat4 _inv_view;
    mat4 _virtual_view;
    mat4 _proj;
    vec2 near_far_plane;
    vec2 _sun_direction;
    vec4 _sun_color;
    mat4 _sun_proj_view;
} ubo;

layout(binding = 1) uniform DepthUniformData {
    mat4 proj_view;
} depth_ubo;

layout(binding = 2) uniform sampler2D height_map;
layout(binding = 5) uniform sampler2D curvature;

layout( push_constant ) uniform constants
{
    mat4 model;
    float tesselation_strength;
    float max_tesselation;
    float height_scale;
    float texture_eps;
    int visualization_mode;
} pc;

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
bool is_culled();

void main()
{
    if (gl_InvocationID == 0)
    {
        if (is_culled()) {
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

bool is_culled() {
    // extract frustum planes in object space https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    mat4 MVP = depth_ubo.proj_view * pc.model;
    
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
    vec3 min_pos = inPos[0];
    min_pos.z = (texture(height_map, inUV[0]).r - eps) * pc.height_scale;

    vec3 max_pos = inPos[0];
    max_pos.z = (texture(height_map, inUV[0]).r + eps) * pc.height_scale;
    
    for (int i=1; i < 4; i++) {
        vec3 pos = inPos[i];
        pos.z = texture(height_map, inUV[i]).r * pc.height_scale;

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

vec4 project_point(vec4 p, vec2 uv) {
    p.z = texture(height_map, uv).r * pc.height_scale;
    vec4 proj_p = depth_ubo.proj_view * pc.model * p;
    proj_p = proj_p / proj_p.w;

    return proj_p;
}

float linearize_depth(float d)
{
    return ubo.near_far_plane.x * ubo.near_far_plane.y / (ubo.near_far_plane.y + d * (ubo.near_far_plane.x - ubo.near_far_plane.y));
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float inout_bezier(float x) {
    return x * x * (3 - 2*x);
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

        float linear_depth = map(linearize_depth(project_point(vec4(inPos[i],1), inUV[i]).z), ubo.near_far_plane.x, ubo.near_far_plane.y, 0, 1);

        res[i] = abs_h / nr_samples;
    }

    return res;
}