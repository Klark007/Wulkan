#version 450
#extension GL_EXT_scalar_block_layout : enable
// frag shader for terrain, supporting cascaded shadow maps and contact hardening soft shadows

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float model_height;

layout(binding = 2) uniform sampler2D albedo;
layout(binding = 3) uniform sampler2D normal_map;
layout(binding = 4) uniform sampler2D height_map;
layout(binding = 5) uniform sampler2D curvature;

layout(binding = 6) uniform texture2DArray shadow_map;
layout(binding = 7) uniform sampler shadow_sampler;
layout(binding = 8) uniform sampler shadow_gather_sampler;

layout(std430, binding = 0) uniform UniformData {
    mat4 view;
    mat4 _inv_view;
    mat4 virtual_view;
    mat4 _proj;
    vec2 near_far_plane;
} ubo;

layout (constant_id = 0) const int cascade_count = 4;

layout(std430, binding = 1) uniform DirectionalLightData {
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


layout( push_constant ) uniform constants
{
    mat4  model;
    float _tesselation_strength;
    float _max_tesselation;
    float _texture_eps;
    int visualization_mode;
    int _cascade_idx;
} pc;

layout(location = 0) out vec4 outColor;

vec3 spherical_to_dir(vec2 sph);

float shadow(uint cascade_idx);
// contact hardening soft shadows based on "Contact-hardening Soft Shadows Made Fast"
float soft_shadow(uint cascade_idx);

float get_penumbra_size(float gradient_noise, vec2 shadow_tex_coord, float receiver_depth,  uint cascade_idx);
// samples sample_idx out of sample_count many samples and rotates by phi
vec2 sample_vogel_disk(int sample_idx, int sample_count, float phi);
// given screen_pos return a random value in [0,1]
float interleaved_gradient_noise(vec2 screen_pos);
float avg_depth_to_penumbra(float receiver_depth, float avg_occluder_depth);
// 1 texel movement for samples differ a lot in world space when switching between cascades
// need to rescale wrt to which cascade the samples are in to have them remain similar sizes in world space
vec2 get_scale_shadow_samples(uint cascade_idx, vec2 texture_size); 

void main() {
    vec3 obj_normal = normalize(texture(normal_map, inUV).rgb * 2 - 1);
    vec3 world_normal = normalize(mat3(transpose((pc.model))) * obj_normal); // normals are in object space not tangent space
    world_normal.y *= -1;

    vec4 view_pos = ubo.virtual_view * vec4(inWorldPos, 1.0); 
    uint cascade_idx = cascade_count;
    for (uint i = 0; i < cascade_count; i++) {
        if (-view_pos.z < directional_light_ubo.cascade_splits[i]) {
            cascade_idx = i;
            break;
        }
    }

    switch (pc.visualization_mode) {
        case 0: // shading    
            float cos_theata =  max(
                dot(
                    world_normal, 
                    spherical_to_dir(directional_light_ubo.light_direction)
                ), 0.1
            );

            
            float in_shadow = 1.0;
            switch (directional_light_ubo.shadow_mode) {
                case 0:
                    in_shadow = 1.0;
                    break;
                case 1:
                    in_shadow = max(shadow(cascade_idx), 0.1);
                    break;
                case 2:
                    in_shadow = max(soft_shadow(cascade_idx), 0.1);
                    break;
                default:
                    in_shadow = 1.0;
                    break;
            }

            outColor = texture(albedo, inUV) * vec4(directional_light_ubo.light_color, 1) * cos_theata * in_shadow;
            break;
        case 1: // tesselation level
            outColor = inColor;
            break;
        case 2: // height
            outColor = texture(height_map, inUV);
            break;
        case 3: // normal
            outColor = vec4(abs(obj_normal), 1);
            break;
        case 4: // error
            float texture_height = texture(height_map, inUV).r;
            float err = (model_height - texture_height);
            float res = abs(err);
            outColor = vec4(vec3(res)*100, 1);
            break;
        case 5: // shadow map cascade
            switch (cascade_idx) {
                case 0:
                    outColor = vec4(1,0,0,1);
                    break;
                case 1:
                    outColor = vec4(1,1,0,1);
                    break;
                case 2:
                    outColor = vec4(0,1,0,1);
                    break;
                case 3:
                    outColor = vec4(0,0,1,1);
                    break;
                case 4:
                    outColor = vec4(1,0,1,1);
                    break;
            }
            break;
        default:
            outColor = vec4(1,0,1,1);
            break;
    }
}

vec3 spherical_to_dir(vec2 sph) {
    float cos_theta = cos(sph.x);
    float sin_theta = sin(sph.x);

    float cos_phi = cos(sph.y);
    float sin_phi = sin(sph.y);

    return vec3(
        sin_theta * cos_phi,
        sin_theta * sin_phi,
        cos_theta
    );
}

float shadow(uint cascade_idx) 
{
    vec4 shadow_coord = directional_light_ubo.proj_views[cascade_idx] * vec4(inWorldPos, 1.0);
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

float soft_shadow(uint cascade_idx) 
{
    vec4 shadow_coord = directional_light_ubo.proj_views[cascade_idx] * vec4(inWorldPos, 1.0);
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

vec2 sample_vogel_disk(int sample_idx, int sample_count, float phi) 
{
    float golden_angle = 2.4f;

    float r = sqrt(sample_idx + 0.5f) / sqrt(sample_count);
    float theta = sample_idx * golden_angle + phi;

    float sine= sin(theta);
    float cosine = cos(theta);

    return vec2(r*cosine, r*sine);
}

float interleaved_gradient_noise(vec2 screen_pos) 
{
    vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(screen_pos, magic.xy)));
}

float avg_depth_to_penumbra(float receiver_depth, float avg_occluder_depth)
{
    return directional_light_ubo.receiver_sample_region * (receiver_depth - avg_occluder_depth) / avg_occluder_depth;
}

float get_penumbra_size(float gradient_noise, vec2 shadow_tex_coord, float receiver_depth, uint cascade_idx) 
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

vec2 get_scale_shadow_samples(uint cascade_idx, vec2 texture_size) {
    // scale to counter how much bigger the same resolution of pixels are in world space
    return directional_light_ubo.shadow_extends[0] / directional_light_ubo.shadow_extends[cascade_idx] / texture_size;
}