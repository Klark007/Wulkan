#version 450

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec4 inColor[];

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

layout(location = 0) out vec3 outWorld_pos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outColor;

void main()
{
    vec2 uv1 = mix(inUV[0], inUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(inUV[3], inUV[2], gl_TessCoord.x);
	outUV = mix(uv1, uv2, gl_TessCoord.y);

    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

    pos.z = texture(height_map, outUV).r * pc.height_scale;

    vec4 world_pos = pc.model * pos;
    outWorld_pos = world_pos.xyz;
    gl_Position = ubo.proj * ubo.view * world_pos;

    // TODO: Check NORMAL INTERPOLATION, won't give actual normals
    vec3 normal1 = normalize(mix(inNormal[0], inNormal[1], gl_TessCoord.x));
    vec3 normal2 = normalize(mix(inNormal[1], inNormal[2], gl_TessCoord.x));
    outNormal = normalize(mix(normal1, normal2, gl_TessCoord.y));

    vec4 color1 = mix(inColor[0], inColor[1], gl_TessCoord.x);
    vec4 color2 = mix(inColor[1], inColor[2], gl_TessCoord.x);
    outColor = texture(height_map, outUV); //mix(color1, color2, gl_TessCoord.y);
}

