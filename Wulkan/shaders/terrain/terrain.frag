#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float model_height;


layout(binding = 2) uniform sampler2D height_map;
layout(binding = 3) uniform sampler2D albedo;
layout(binding = 4) uniform sampler2D normal_map;
layout(binding = 5) uniform sampler2D curvature;
layout(binding = 6) uniform sampler2DArray shadow_map;

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 _inv_view;
    mat4 virtual_view;
    mat4 _proj;
    vec2 near_far_plane;
    vec2 sun_direction;
    vec4 sun_color;
} ubo;

#define SHADOW_MAP_CASCADE_COUNT 4

layout(binding = 1) uniform CascadeUniformData {
    mat4 proj_views[SHADOW_MAP_CASCADE_COUNT];
    vec4 cascade_splits;
} depth_ubo;


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
float linearize_depth(float d);
float shadow(uint cascade_idx);

void main() {
    vec3 obj_normal = normalize(texture(normal_map, inUV).rgb * 2 - 1);
    vec3 world_normal = normalize(mat3(transpose((pc.model))) * obj_normal); // normals are in object space not tangent space
    world_normal.y *= -1;

    vec4 view_pos = ubo.virtual_view * vec4(inWorldPos, 1.0); 
    uint cascade_idx = 4;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
        if (-view_pos.z < depth_ubo.cascade_splits[i]) {
            cascade_idx = i;
            break;
        }
    }

    switch (pc.visualization_mode) {
        case 0: // shading    
            float cos_theata =  max(
                dot(
                    world_normal, 
                    spherical_to_dir(ubo.sun_direction)
                ), 0.1
            );

            float in_shadow = shadow(cascade_idx);

            /* Debugging
            if (in_shadow < 1) {
                // in shadow
                outColor = vec4(1,0,0,1);
            } else if (in_shadow > 1) {
                // in sun
                outColor = texture(albedo, inUV) * cos_theata; //* in_shadow;
            } else {
                outColor = vec4(1,0,1,1);

            }
            */
            
            outColor = texture(albedo, inUV) * ubo.sun_color * cos_theata * in_shadow;
            break;
        case 1: // tesselation level
            outColor = inColor;
            break;
        case 2: // height
            outColor = texture(height_map, inUV);
            break;
        case 3: // normal
            outColor = vec4(abs(obj_normal), 1);//texture(normal_map, inUV);
            break;
        case 4: // error
            float texture_height = texture(height_map, inUV).r;
            float err = (model_height - texture_height);
            float res = abs(err);
            outColor = vec4(vec3(res)*100, 1);
            break;
        case 5: // linear depth from shadow map
            vec4 s_coord = depth_ubo.proj_views[cascade_idx] * vec4(inWorldPos, 1.0);
            s_coord /= s_coord.w;
            if (
                -1 <= s_coord.x && s_coord.x <= 1 && 
                -1 <= s_coord.y && s_coord.y <= 1 &&
                -1 <= s_coord.z && s_coord.z <= 1 &&
                0 <= s_coord.w
            ) {
                vec2 texCoord = (s_coord.xy + vec2(1)) / 2;
                outColor = vec4(vec3(linearize_depth(texture(shadow_map, vec3(texCoord,cascade_idx)).r)), 1);
            } else {
                outColor = vec4(1,0,1,1);
            }
            break;
        case 6: // shadow map cascade
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

float shadow(uint cascade_idx) {
    vec4 shadow_coord = depth_ubo.proj_views[cascade_idx] * vec4(inWorldPos, 1.0);
    shadow_coord /= shadow_coord.w;

    if (
        -1 <= shadow_coord.x && shadow_coord.x <= 1 && 
        -1 <= shadow_coord.y && shadow_coord.y <= 1 &&
        -1 <= shadow_coord.z && shadow_coord.z <= 1
    ) {
        vec2 texCoord = (shadow_coord.xy + vec2(1)) / 2;
        float dist = texture(shadow_map, vec3(texCoord, cascade_idx)).r;

        // bias to avoid acne
        if (dist <= shadow_coord.z) {
            // in shadow
            return 0.1;
        }
    }

    return 1;
}

float linearize_depth(float d)
{
    return ubo.near_far_plane.x * ubo.near_far_plane.y / (ubo.near_far_plane.y + d * (ubo.near_far_plane.x - ubo.near_far_plane.y));
}