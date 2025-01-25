#version 450

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec4 inColor[];

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 virtual_view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform sampler2D height_map;

layout( push_constant ) uniform constants
{
    mat4 model;
    float tesselation_strength;
    float height_scale;
} pc;

layout (vertices = 4) out;

layout(location = 0) out vec2 outUV[4];
layout(location = 1) out vec3 outNormal[4];
layout(location = 2) out vec4 outColor[4];

//float local_tesselation_strength(uint idx);
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
            float ls0 = 1;//local_tesselation_strength(0);
            float ls1 = 1;//local_tesselation_strength(1);
            float ls2 = 1;//local_tesselation_strength(2);
            float ls3 = 1;//local_tesselation_strength(3);

            gl_TessLevelOuter[0] = mix(ls3, ls0, 0.5) * pc.tesselation_strength;
            gl_TessLevelOuter[1] = mix(ls0, ls1, 0.5) * pc.tesselation_strength;
            gl_TessLevelOuter[2] = mix(ls1, ls2, 0.5) * pc.tesselation_strength;
            gl_TessLevelOuter[3] = mix(ls2, ls3, 0.5) * pc.tesselation_strength;

            gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
            gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);
        }
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;

    outUV[gl_InvocationID] = inUV[gl_InvocationID];
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    outColor[gl_InvocationID] = inColor[gl_InvocationID];
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
    vec3 pos_zero = inPos[0];
    pos_zero.z = texture(height_map, inUV[0]).r * pc.height_scale;

    vec3 min_pos = pos_zero;
    vec3 max_pos = pos_zero;

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


float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float f_der_y(vec2 uv, vec2 dir, float eps) {
    // for dydu dir == vec2(1,0) for dydv dir == vec2(0,1)
    return (texture(height_map, uv + eps*dir).r - texture(height_map, uv - eps*dir).r) / (2*eps);
}

float s_der_y(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    return (f_der_y(uv + eps2*dir2, dir1, eps1) - f_der_y(uv - eps2*dir2, dir1, eps1)) / (2*eps2);
}

const float smoothness_factor = 50.0;
float normalized_derivative(float der, float eps) {
    return clamp(map(abs(der),0,1.0/(smoothness_factor * 2*eps), 0, 1), 0, 1);
}

float normalized_s_der(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    float f_der1 = f_der_y(uv + eps2*dir2, dir1, eps1);
    float f_der2 = f_der_y(uv - eps2*dir2, dir1, eps1);

    return normalized_derivative((f_der1-f_der2) / (2*eps2), eps1*eps2);
}

// TODO: Set by uniform buffer object or extracted from projection matrix
const float zNear = 0.1;
const float zFar  = 100.0;
float linearize_depth(float d)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

float inout_bezier(float x) {
    return x * x * (3 - 2*x);
}

float local_tesselation_strength(uint idx) {
    // returns scalar between 0 and 1
    vec2 texSize = textureSize(height_map,0);
    vec2 epsilon = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    float n_ddydduu = normalized_s_der(inUV[idx], vec2(1,0), vec2(1,0), epsilon[0], epsilon[0]);
    float n_ddyddvv = normalized_s_der(inUV[idx], vec2(0,1), vec2(0,1), epsilon[1], epsilon[1]);
    float n_ddydduv = normalized_s_der(inUV[idx], vec2(1,0), vec2(0,1), epsilon[0], epsilon[1]);
    float n_ddyddvu = normalized_s_der(inUV[idx], vec2(1,0), vec2(1,0), epsilon[1], epsilon[0]);
    
    float ddydduu = s_der_y(inUV[idx], vec2(1,0), vec2(1,0), epsilon[0], epsilon[0]);
    float ddyddvv = s_der_y(inUV[idx], vec2(0,1), vec2(0,1), epsilon[1], epsilon[1]);
    float ddydduv = s_der_y(inUV[idx], vec2(1,0), vec2(0,1), epsilon[0], epsilon[1]);
    float dydu = f_der_y(inUV[idx], vec2(1,0), epsilon[0]);
    float dydv = f_der_y(inUV[idx], vec2(0,1), epsilon[1]);


    vec4 mod_pos = vec4(inPos[idx],1);
    mod_pos.y = texture(height_map, inUV[idx]).r * pc.height_scale;
    vec4 pos = ubo.proj * ubo.virtual_view * pc.model * mod_pos;
    pos /= pos.w;
    
    if (!(-1 <= pos.x && pos.x <= 1) || !(-1 <= pos.y && pos.y <= 1) || !(0 <= pos.z && pos.z <= 1)) {
        return 0;
    }

    float linear_f_depth = map(linearize_depth(pos.z), zNear, zFar, 0, 1);
    float depth = inout_bezier(1.0 - linear_f_depth);

    float approach_1 = sqrt(n_ddydduu * n_ddydduu + n_ddyddvv * n_ddyddvv) / sqrt(2);
    float approach_2 = sqrt(n_ddydduu * n_ddydduu + n_ddyddvv * n_ddyddvv + n_ddydduv * n_ddydduv + n_ddyddvu * n_ddyddvu) / sqrt(4);
    float approach_macu = 0.01 * abs((ddydduu * ddyddvv - ddydduv*ddydduv) / pow(1 + dydu*dydu + dydv * dydv, 2));
    float approach_3 = 0.5 * approach_2 + 0.5 * depth;


    return approach_macu;
}