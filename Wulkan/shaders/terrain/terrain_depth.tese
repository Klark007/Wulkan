#version 450

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];

#define MAX_CASCADE_COUNT 4
layout(binding = 1) uniform DepthUniformData {
    mat4 proj_views[MAX_CASCADE_COUNT];
    float _cascade_splits[MAX_CASCADE_COUNT];
} depth_ubo;

layout(binding = 2) uniform sampler2D height_map;


layout( push_constant ) uniform constants
{
    mat4 model;
    float _tesselation_strength;
    float _max_tesselation;
    float _texture_eps;
    int _visualization_mode;
    int cascade_idx;
} pc;

void main()
{
    vec2 uv1 = mix(inUV[0], inUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(inUV[3], inUV[2], gl_TessCoord.x);
	vec2 outUV = mix(uv1, uv2, gl_TessCoord.y);

    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

    pos.z = texture(height_map, outUV).r;

    gl_Position = depth_ubo.proj_views[pc.cascade_idx] * pc.model * pos;
}

