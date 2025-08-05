#version 450
#extension GL_EXT_scalar_block_layout : enable
// depth only version of terrain.tese

layout (quads, fractional_odd_spacing, cw) in;
layout(location = 0) in vec2 inUV[];

layout (constant_id = 0) const int cascade_count = 4;
layout(std430, binding = 1) uniform DirectionalLightData {
    mat4 proj_views[cascade_count];
    vec4 _cascade_splits;
    vec2 _shadow_extends[cascade_count]; 
    vec3 _light_color; 
    float _receiver_sample_region; 
    vec2 _light_direction;
    float _occluder_sample_region; 
    int _nr_shadow_receiver_samples;
    int _nr_shadow_occluder_samples;
} directional_light_ubo;

layout(binding = 4) uniform sampler2D height_map;

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

    gl_Position = directional_light_ubo.proj_views[pc.cascade_idx] * pc.model * pos;
}

