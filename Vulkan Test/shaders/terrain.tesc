#version 450


layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float tesselationStrength;
    float heightScale;
} ubo;

layout(binding = 1) uniform sampler2D heightMap;

layout (vertices = 4) out;

layout(location = 0) in vec2 inUV[];
layout(location = 0) out vec2 outUV[4];
layout(location = 1) out float outStrength[4];

float local_tesselation_strength(uint idx);

void main()
{
    if (gl_InvocationID == 0)
    {
        float ls0 = local_tesselation_strength(0);
        float ls1 = local_tesselation_strength(1);
        float ls2 = local_tesselation_strength(2);
        float ls3 = local_tesselation_strength(3);

        gl_TessLevelOuter[0] = mix(ls3, ls0, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[1] = mix(ls0, ls1, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[2] = mix(ls1, ls2, 0.5) * ubo.tesselationStrength;
        gl_TessLevelOuter[3] = mix(ls2, ls3, 0.5) * ubo.tesselationStrength;

        gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
        gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);

        outStrength[gl_InvocationID] = ls0;
    } else {
        outStrength[gl_InvocationID] = local_tesselation_strength(gl_InvocationID);
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

/*
mat2 jacobian(vec2 uv, vec2 eps) {
}
*/

const float smoothness_factor = 50.0;
float normalized_derivative(float der, float eps) {
    return clamp(map(abs(der),0,1.0/(smoothness_factor * 2*eps), 0, 1), 0, 1);
}

float normalized_s_der(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    float f_der1 = f_der_y(uv + eps2*dir2, dir1, eps1);
    float f_der2 = f_der_y(uv - eps2*dir2, dir1, eps1);

    return normalized_derivative((f_der1-f_der2) / (2*eps2), eps1*eps2);
}

float local_tesselation_strength(uint idx) {
    // returns scalar between 0 and 1
    vec2 texSize = textureSize(heightMap,0);
    vec2 epsilon = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    float ddydduu = normalized_s_der(inUV[idx], vec2(1,0), vec2(1,0), epsilon[0], epsilon[0]);
    float ddyddvv = normalized_s_der(inUV[idx], vec2(0,1), vec2(0,1), epsilon[1], epsilon[1]);

    return sqrt(ddydduu * ddydduu + ddyddvv * ddyddvv) / sqrt(2);
}
