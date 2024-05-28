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


void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = ubo.tesselationStrength;
        gl_TessLevelInner[1] = ubo.tesselationStrength;

        gl_TessLevelOuter[0] = ubo.tesselationStrength;
        gl_TessLevelOuter[1] = ubo.tesselationStrength;
        gl_TessLevelOuter[2] = ubo.tesselationStrength;
        gl_TessLevelOuter[3] = ubo.tesselationStrength;
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
    outUV[gl_InvocationID] = inUV[gl_InvocationID];
}
