#version 450
// PBR implementation
#extension GL_EXT_demote_to_helper_invocation : require // to use discard

#include "../common.shader"
#include "shadow.shader"
#include "pbr_common.shader"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
	// stored in the upper 16 bits of pbr_uniforms.configuration
	uint visualization_mode = pbr_uniforms.configuration >> 16;

	switch (visualization_mode) {
        case 0: // shading    
			{
				vec3 w_i = spherical_to_dir(directional_light_ubo.light_direction); // incoming / light dir
				vec3 n   = normalize(inWorldNormal);
	
				vec3 camera_pos = vec3(ubo.inv_view[3][0], ubo.inv_view[3][1], ubo.inv_view[3][2]);
				vec3 w_o = normalize(camera_pos - inWorldPos); // view direction

				float in_shadow = shadow(inWorldPos);
	
				vec4 pbr_res = pbr(w_i, w_o, n, directional_light_ubo.light_color, inUV, in_shadow);

				// pbr frag does not support semitransparent geometry. Only fully transparant
				if (pbr_res.a < 0.2) {
					discard;
				}
				outColor = pbr_res;
			}
			break;
		case 1: // normal
            outColor = vec4(abs(normalize(inWorldNormal)), 1);
            break;
		case 2: // diffuse color
			uint use_diffuse_texture = pbr_uniforms.configuration & 1<<0;
			vec3 diffuse_col = texture(diffuse_tex, inUV).rgb;
	
			outColor = vec4((1 - use_diffuse_texture) *  pbr_uniforms.diffuse + use_diffuse_texture * diffuse_col, 1);
            break;
		case 3: // cascade idx
			vec4 view_pos = ubo.virtual_view * vec4(inWorldPos, 1.0); 

            uint cascade_idx = cascade_count;
            for (uint i = 0; i < cascade_count; i++) {
                if (-view_pos.z < directional_light_ubo.cascade_splits[i]) {
                    cascade_idx = i;
                    break;
                }
            }
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