#version 450

layout(location = 0) in float model_height;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;

layout(binding = 1) uniform sampler2D height_map;
layout(binding = 2) uniform sampler2D normal_map;

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

void main() {
    switch (pc.visualization_mode) {
        case 0: // shading
            outColor = inColor * dot(
                normalize(texture(normal_map, inUV).rgb), 
                normalize(vec3(0, 0, 1))
            );
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