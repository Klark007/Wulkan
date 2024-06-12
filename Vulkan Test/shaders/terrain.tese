#version 450

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];
layout(location = 1) in float inStrength[];


layout(location = 0) out vec2 outUV;
layout(location = 1) out float outStrength;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float tesselationStrength; // push constants?
    float heightScale; // push constants?
} ubo;

layout(binding = 1) uniform sampler2D heightMap;

void main()
{
    vec2 uv1 = mix(inUV[0], inUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(inUV[3], inUV[2], gl_TessCoord.x);
	outUV = mix(uv1, uv2, gl_TessCoord.y);

    if (gl_TessCoord.x < 0.5 && gl_TessCoord.y < 0.5) {
        outStrength = inStrength[0];
    } else if (gl_TessCoord.x >= 0.5 && gl_TessCoord.y < 0.5) {
        outStrength = inStrength[1];
    } else if (gl_TessCoord.x < 0.5 && gl_TessCoord.y >= 0.5) {
        outStrength = inStrength[3];
    } else {
        outStrength = inStrength[2];
    }

    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

    pos.y += texture(heightMap, outUV).r * ubo.heightScale;

    gl_Position = ubo.proj * ubo.view * ubo.model * pos;
}