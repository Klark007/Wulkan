#version 450

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec4 inColor[];

layout(binding = 1) uniform UniformData {
    mat4 proj_view;
} depth_ubo;

layout(binding = 2) uniform sampler2D height_map;


layout( push_constant ) uniform constants
{
    mat4 model;
    float _tesselation_strength;
    float _max_tesselation;
    float height_scale;
    float _texture_eps;
    int _visualization_mode;
} pc;

void main()
{
    vec2 uv1 = mix(inUV[0], inUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(inUV[3], inUV[2], gl_TessCoord.x);
	vec2 outUV = mix(uv1, uv2, gl_TessCoord.y);

    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

    pos.z = texture(height_map, outUV).r * pc.height_scale;

    gl_Position = depth_ubo.proj_view * pc.model * pos;
}

