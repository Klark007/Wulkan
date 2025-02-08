#version 450

layout(location = 0) in vec3 inUVW;

layout(binding = 1) uniform samplerCube cube_map;

layout(location = 0) out vec4 outColor;

void main() {
    // cube maps in vulkan are y down but engine is z up
    outColor = texture(cube_map, inUVW.xzy);
}