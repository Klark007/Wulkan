#version 450
// frag shader for terrain, supporting cascaded shadow maps and contact hardening soft shadows

#include "../common.shader"
#include "terrain_common.shader"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in float model_height;

// requires the above uniforms to be defined
#include "../pbr/shadow.shader"

layout(location = 0) out vec4 outColor;

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
                    in_shadow = max(shadow(inWorldPos, cascade_idx), 0.1);
                    break;
                case 2:
                    in_shadow = max(soft_shadow(inWorldPos, cascade_idx), 0.1);
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
