#version 450

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float tesselationStrength;
    float heightScale;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;


layout(location = 0) out vec4 outColor;


void main() {
    float gt = texture(texSampler, texCoord).r * ubo.heightScale;
    float v  = worldPos.y;
    float err = abs(gt-v);
    outColor = vec4(err*1.2);
}