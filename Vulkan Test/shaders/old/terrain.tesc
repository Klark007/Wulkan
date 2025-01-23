#version 450

layout(location = 0) in vec4 inPos[];
layout(location = 1) in vec2 inUV[];

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 virtualView;
    mat4 proj;
    float tesselationStrength;
    float heightScale;
} ubo;

layout(binding = 1) uniform sampler2D heightMap;

layout (vertices = 4) out;

layout(location = 0) out vec2 outUV[4];

float local_tesselation_strength(uint idx);

void main()
{
    if (gl_InvocationID == 0)
    {
        float ls0 = 1;//local_tesselation_strength(0);
        float ls1 = 1;//local_tesselation_strength(1);
        float ls2 = 1;//local_tesselation_strength(2);
        float ls3 = 1;//local_tesselation_strength(3);


        bool all_zero = (ls0 == 0) && (ls1 == 0) && (ls2 == 0) && (ls3 == 0);
        float eps = (ls0 + ls1 + ls2 + ls3) / 4;
        if (!all_zero) {
            if (ls0 == 0) {
                ls0 = eps;
            }
            if (ls1 == 0) {
                ls1 = eps;
            }
            if (ls2 == 0) {
                ls2 = eps;
            }
            if (ls3 == 0) {
                ls3 = eps;
            }
        }

        gl_TessLevelOuter[0] = mix(ls3, ls0, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[1] = mix(ls0, ls1, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[2] = mix(ls1, ls2, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[3] = mix(ls2, ls3, 0.5) * ubo.tesselationStrength;

        gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
        gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);

    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
    outUV[gl_InvocationID] = inUV[gl_InvocationID];
}


float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float f_der_y(vec2 uv, vec2 dir, float eps) {
    // for dydu dir == vec2(1,0) for dydv dir == vec2(0,1)
    return (texture(heightMap, uv + eps*dir).r - texture(heightMap, uv - eps*dir).r) / (2*eps);
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
    vec2 texSize = textureSize(heightMap,0);
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


    vec4 mod_pos = inPos[idx];
    mod_pos.y = texture(heightMap, inUV[idx]).r * ubo.heightScale;
    vec4 pos = ubo.proj * ubo.virtualView * ubo.model * mod_pos;
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
