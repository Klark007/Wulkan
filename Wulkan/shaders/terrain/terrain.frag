#version 450

layout(location = 0) in float model_height;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;

layout(binding = 2) uniform sampler2D height_map;
layout(binding = 3) uniform sampler2D albedo;
layout(binding = 4) uniform sampler2D normal_map;
layout(binding = 5) uniform sampler2D curvature;

layout(binding = 0) uniform UniformData {
    mat4 _view;
    mat4 _inv_view;
    mat4 _virtual_view;
    mat4 _proj;
    vec2 _near_far_plane;
    vec2 sun_direction;
    vec4 sun_color;
} ubo;

layout( push_constant ) uniform constants
{
    mat4  model;
    float _tesselation_strength;
    float _max_tesselation;
    float height_scale;
    float _texture_eps;
    int visualization_mode;
} pc;

layout(location = 0) out vec4 outColor;

vec3 spherical_to_dir(vec2 sph);

void main() {
    vec3 obj_normal = normalize((texture(normal_map, inUV).rgb - vec3(0.5, 0.5, 0)) * vec3(2,2,1));
    vec3 world_normal = normalize(mat3(transpose((pc.model))) * obj_normal); // normals are in object space not tangent space

    switch (pc.visualization_mode) {
        case 0: // shading      
            float cos_theata =  max(
                dot(
                    world_normal, 
                    spherical_to_dir(ubo.sun_direction)
                ), 0
            );

            vec4 ambient = texture(albedo, inUV) * 0.05;
            outColor = texture(albedo, inUV) * ubo.sun_color * cos_theata + ambient;
            break;
        case 1: // tesselation level
            outColor = inColor;
            break;
        case 2: // height
            outColor = texture(height_map, inUV);
            break;
        case 3: // normal
            outColor = texture(normal_map, inUV);
            break;
        case 4: // error
            float texture_height = texture(height_map, inUV).r * pc.height_scale;
            float err = (model_height - texture_height) / pc.height_scale;
            float res = abs(err);
            outColor = vec4(vec3(res)*100, 1);
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