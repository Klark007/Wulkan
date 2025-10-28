#ifndef SHADOW_COMMON_INCLUDE
#define SHADOW_COMMON_INCLUDE

layout(constant_id = 0) const int cascade_count = 4;
layout(std430, set = 1, binding = 0) uniform DirectionalLightData {
    mat4 proj_views[cascade_count];
    vec4 cascade_splits;
    vec2 shadow_extends[cascade_count];  // phyical extends of orthographic projections horizontaly and vertically
    vec3 light_color;                    // already scaled by intensity
    float receiver_sample_region;        // how much the samples for if we are in shadow are distributed
    vec2 light_direction;
    float occluder_sample_region;        // how much the samples for average occluder distance are distributed 
    int nr_shadow_receiver_samples;      // isn't as performant as specialization consts or const, but want to be able to play with these settings
    int nr_shadow_occluder_samples;      
    int shadow_mode;				     // 0: no shadows, 1: hard shadows, 2: soft shadows 
} directional_light_ubo;

layout(set = 1, binding = 1) uniform texture2DArray shadow_map;
layout(set = 1, binding = 2) uniform sampler shadow_sampler;
layout(set = 1, binding = 3) uniform sampler shadow_gather_sampler;

#ifndef SHADOW_UNIFORM_ONLY

#include "../sampling.shader"

float hard_shadow(vec3 world_pos, uint cascade_idx);
float soft_shadow(vec3 world_pos, uint cascade_idx);

float shadow(vec3 world_pos) 
{
    vec4 view_pos = ubo.virtual_view * vec4(world_pos, 1.0); 

    // compute cascade idx
    uint cascade_idx = cascade_count;
    for (uint i = 0; i < cascade_count; i++) {
        if (-view_pos.z < directional_light_ubo.cascade_splits[i]) {
            cascade_idx = i;
            break;
        }
    }

    float in_shadow = 1.0;
    switch (directional_light_ubo.shadow_mode) {
        case 0:
            in_shadow = 1.0;
            break;
        case 1:
            in_shadow = hard_shadow(world_pos, cascade_idx);
            break;
        case 2:
            in_shadow = soft_shadow(world_pos, cascade_idx);
            break;
        default:
            in_shadow = 1.0;
            break;
    }

    return in_shadow;
}

float hard_shadow(vec3 world_pos, uint cascade_idx) 
{
    vec4 shadow_coord = directional_light_ubo.proj_views[cascade_idx] * vec4(world_pos, 1.0);
    shadow_coord /= shadow_coord.w;

    if (
        -1 <= shadow_coord.x && shadow_coord.x <= 1 && 
        -1 <= shadow_coord.y && shadow_coord.y <= 1 &&
        -1 <= shadow_coord.z && shadow_coord.z <= 1
    ) {
        vec2 texCoord = (shadow_coord.xy + vec2(1)) / 2;
        
        float compare_value = max(textureGather(sampler2DArrayShadow(shadow_map, shadow_gather_sampler), vec3(texCoord, cascade_idx), shadow_coord.z).r, 0.1); // true (1) if texture depth > shadow_coord.z else 0        
        return compare_value;
    }

    return 1;
}

float avg_depth_to_penumbra(float receiver_depth, float avg_occluder_depth)
{
    return directional_light_ubo.receiver_sample_region * (receiver_depth - avg_occluder_depth) / avg_occluder_depth;
}

// 1 texel movement for samples differ a lot in world space when switching between cascades
// need to rescale wrt to which cascade the samples are in to have them remain similar sizes in world space
vec2 get_scale_shadow_samples(uint cascade_idx, vec2 texture_size)
{
    // scale to counter how much bigger the same resolution of pixels are in world space
    return directional_light_ubo.shadow_extends[0] / directional_light_ubo.shadow_extends[cascade_idx] / texture_size;
}

float get_penumbra_size(float gradient_noise, vec2 shadow_tex_coord, float receiver_depth,  uint cascade_idx)
{
    float avg_occluder_depth = 0.0f;
    int nr_occluders = 0;

    for (int i = 0; i < directional_light_ubo.nr_shadow_occluder_samples; i++) {
        vec2 tex_coord = sample_vogel_disk(i, directional_light_ubo.nr_shadow_occluder_samples, gradient_noise);
        vec2 scale = get_scale_shadow_samples(cascade_idx, textureSize(sampler2DArray(shadow_map, shadow_sampler), 0).xy);

        tex_coord = shadow_tex_coord + tex_coord * directional_light_ubo.occluder_sample_region * scale;

        float depth = texture(sampler2DArray(shadow_map, shadow_sampler), vec3(tex_coord, cascade_idx)).r;

        // is actually an occluder
        if (depth < receiver_depth) {
            avg_occluder_depth += depth;
            nr_occluders += 1;
        }
    }

    if (nr_occluders > 0) {
        avg_occluder_depth /= nr_occluders;
        return avg_depth_to_penumbra(receiver_depth, avg_occluder_depth);
    } else {
        return 0.0f;
    }
}

// contact hardening soft shadows based on "Contact-hardening Soft Shadows Made Fast"
float soft_shadow(vec3 world_pos, uint cascade_idx) {
    vec4 shadow_coord = directional_light_ubo.proj_views[cascade_idx] * vec4(world_pos, 1.0);
    shadow_coord /= shadow_coord.w;

    if (
        -1 <= shadow_coord.x && shadow_coord.x <= 1 && 
        -1 <= shadow_coord.y && shadow_coord.y <= 1 &&
        -1 <= shadow_coord.z && shadow_coord.z <= 1
    ) {
        
        vec2 tex_coord = (shadow_coord.xy + vec2(1)) / 2;

        // our samples will be rotated by a value given the pixel's screen pos
        float gradient_noise = interleaved_gradient_noise(gl_FragCoord.xy);
        float receiver_depth = shadow_coord.z;

        float penumbra_filter_scale = get_penumbra_size(gradient_noise, tex_coord, receiver_depth, cascade_idx);

        // average how much of our samples are in shadow
        float shadow_value = 0;

        // only need to sample multiple times, if we actually have non zero penumbra scale
        int nr_samples = (penumbra_filter_scale > 0) ? directional_light_ubo.nr_shadow_receiver_samples : 1;
        
        for (int i = 0; i < nr_samples; i++) {
            vec2 sample_coord = sample_vogel_disk(i, nr_samples, gradient_noise);

            // scale for better consistency between different cascades
            vec2 scale = get_scale_shadow_samples(cascade_idx, textureSize(sampler2DArrayShadow(shadow_map, shadow_gather_sampler), 0).xy);

            sample_coord = tex_coord + sample_coord * penumbra_filter_scale * scale;

            // true (1) if texture depth > shadow_coord.z else 0  
            shadow_value += textureGather(sampler2DArrayShadow(shadow_map, shadow_gather_sampler), vec3(sample_coord, cascade_idx), shadow_coord.z).r;
        }
        
        return shadow_value / nr_samples;
    }

    // by default not in shadow
    return 1;
}
#endif

#endif