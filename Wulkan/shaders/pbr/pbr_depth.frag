#version 450
// PBR implementation
#extension GL_EXT_demote_to_helper_invocation : require // to use discard

#include "../common.shader"
#include "pbr_common.shader"

layout (location = 0) in vec2 inUV;

void main() 
{
	if (texture(diffuse_tex, inUV).a == 0) {
		discard;
	}
}