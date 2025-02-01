#version 450

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec4 inColor[];

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 _inv_view;
    mat4 virtual_view;
    mat4 proj;
    vec2 near_far_plane;
} ubo;

layout(binding = 1) uniform sampler2D height_map;

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

            gl_TessLevelOuter[0] = min(max(mix(str.w, str.x, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[1] = min(max(mix(str.x, str.y, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[2] = min(max(mix(str.y, str.z, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[3] = min(max(mix(str.z, str.w, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);

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
    mat4 MVP = ubo.proj * ubo.virtual_view * pc.model;
    
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

float f_der_y(vec2 uv, vec2 dir, float eps) {
    // for dydu dir == vec2(1,0) for dydv dir == vec2(0,1)
    return (texture(height_map, uv + eps*dir).r - texture(height_map, uv - eps*dir).r) / (2*eps);
}

float s_der_y(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    
    return (
        texture(height_map, uv + eps1*2*dir1 + eps2*2*dir2).r
        - 2 * texture(height_map, uv).r
        + texture(height_map, uv - eps1*2*dir1 - eps2*2*dir2).r
    ) / (eps1*2 * eps2*2);
    

    //return (f_der_y(uv + eps2*dir2, dir1, eps1) - f_der_y(uv - eps2*dir2, dir1, eps1)) / (2*eps2);
}

vec4 project_point(vec4 p) {
    p.z = texture(height_map, inUV[0]).r * pc.height_scale;
    vec4 proj_p = ubo.proj * ubo.virtual_view * pc.model * p;
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

float projected_size() {
    vec4 p1 = project_point(vec4(inPos[0],1));
    vec4 p2 = project_point(vec4(inPos[1],1));
    vec4 p3 = project_point(vec4(inPos[2],1));
    vec4 p4 = project_point(vec4(inPos[3],1));

    // area of 0,1,2
    float area1 = abs(
        p1.x * (p2.y - p3.y)
        + p2.x * (p3.y - p1.y)
        + p3.x * (p1.y - p2.y)
    ) / 2;

    // area of 0,2,3
    float area2 = abs(
        p1.x * (p2.y - p4.y)
        + p2.x * (p4.y - p1.y)
        + p4.x * (p1.y - p2.y)
    ) / 2;

    return area1 + area2;
}

vec4 compute_tesselation_level() {
    // 1 pixel distance sample is a bad estimate of derivatives / curvature at a patches vertex
    vec2 epsilon = vec2(inUV[2] - inUV[0]) * pc.texture_eps;

    vec4 res = vec4(0);

    for (int i = 0; i < 4; i++) {
        float huu = s_der_y(inUV[i], vec2(1,0), vec2(1,0), epsilon[0], epsilon[0]);
        float hvv = s_der_y(inUV[i], vec2(0,1), vec2(0,1), epsilon[1], epsilon[1]);
        float huv = s_der_y(inUV[i], vec2(1,0), vec2(0,1), epsilon[0], epsilon[1]);
        
        float hu = f_der_y(inUV[i], vec2(1,0), epsilon[0]);
        float hv = f_der_y(inUV[i], vec2(0,1), epsilon[1]);

        // definition taken from https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/adaptive-terrain-tessellation.pdf
        // mean curvature
        float H = (huu * (1 + hv*hv) - 2*huv*hu*hv + hvv * (1 + hu*hu)) / (2*pow(1 + hu*hu + hv*hv, 1.5));
        // gaussian curvature
        //float K = (huu + hvv - huv*huv) / (1 + hu*hu + hv*hv);
        
        float linear_depth = map(linearize_depth(project_point(vec4(inPos[i],1)).z), ubo.near_far_plane.x, ubo.near_far_plane.y, 0, 1);

        res[i] = abs(H) * inout_bezier(1 - linear_depth);
    }

    // Introduces seams
    // res = 0.3 * tesselation_lvl + 0.7 * vec4(projected_size() * 150 * 150);

    return res;
}