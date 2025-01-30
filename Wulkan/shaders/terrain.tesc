#version 450

layout(location = 0) in vec3 inPos[];
layout(location = 1) in vec2 inUV[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec4 inColor[];

layout(binding = 0) uniform UniformData {
    mat4 view;
    mat4 inv_view;
    mat4 virtual_view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform sampler2D height_map;

layout( push_constant ) uniform constants
{
    mat4 model;
    float tesselation_strength;
    float max_tesselation;
    float height_scale;
    float texture_eps;
    int visualization_mode;
} pc;

layout (vertices = 4) out;

layout(location = 0) out vec2 outUV[4];
layout(location = 1) out vec3 outNormal[4];
layout(location = 2) out vec4 outColor[4];

vec4 debug_level(float str) {
    if (str < pc.max_tesselation / 4) {
        return vec4(0,1,0,1);
    } else if (str < pc.max_tesselation / 2) {
        return vec4(0,1,1,1);
    } else if (str < 3 * pc.max_tesselation / 4) {
        return vec4(0,0,1,1);
    } else {
        return vec4(1,0,0,1);
    }
}

vec4 compute_tesselation_level();
bool is_culled();

void main()
{
    if (gl_InvocationID == 0)
    {
        if (is_culled()) {
            gl_TessLevelOuter[0] = 0;
            gl_TessLevelOuter[1] = 0;
            gl_TessLevelOuter[2] = 0;
            gl_TessLevelOuter[3] = 0;
                                   
            gl_TessLevelInner[0] = 0;
            gl_TessLevelInner[1] = 0;
        } else {
            vec4 str = compute_tesselation_level();
            float ls0 = str.x;
            float ls1 = str.y;
            float ls2 = str.z;
            float ls3 = str.w;

            gl_TessLevelOuter[0] = min(max(mix(ls3, ls0, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[1] = min(max(mix(ls0, ls1, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[2] = min(max(mix(ls1, ls2, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);
            gl_TessLevelOuter[3] = min(max(mix(ls2, ls3, 0.5), 1) * pc.tesselation_strength, pc.max_tesselation);

            gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
            gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);
        }
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;

    outUV[gl_InvocationID] = inUV[gl_InvocationID];
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    
    switch (pc.visualization_mode) {
        case 0: // shading
            outColor[gl_InvocationID] = inColor[gl_InvocationID];
            break;
        case 1: // tesselation level
            outColor[gl_InvocationID] = debug_level(min(compute_tesselation_level() * pc.tesselation_strength, pc.max_tesselation)[gl_InvocationID]);
            break;
        default: // Not implemented modes or modes where color is computed in the fragment shader (pink) 
            outColor[gl_InvocationID] = vec4(1,0,1,1);
            break;
    } 
}

bool is_culled() {
    // extract frustum planes in object space https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    mat4 MVP = ubo.proj * ubo.virtual_view * pc.model;
    
    vec4 left = vec4(0);
    for (int i = 0; i < 4; i++) {
        left[i] = MVP[i][3] + MVP[i][0];
    }
    
    vec4 right = vec4(0);
    for (int i = 0; i < 4; i++) {
        right[i] = MVP[i][3] - MVP[i][0];
    }

    vec4 bottom = vec4(0);
    for (int i = 0; i < 4; i++) {
        bottom[i] = MVP[i][3] + MVP[i][1];
    }

    vec4 top = vec4(0);
    for (int i = 0; i < 4; i++) {
        top[i] = MVP[i][3] - MVP[i][1];
    }

    vec4 near = vec4(0);
    for (int i = 0; i < 4; i++) {
        near[i] = MVP[i][3] + MVP[i][2];
    }

    vec4 far = vec4(0);
    for (int i = 0; i < 4; i++) {
        far[i] = MVP[i][3] - MVP[i][2];
    }
    
    vec4 planes[6] = vec4[6](left, right, bottom, top, near, far);


    // bounding box
    float eps = 0.01; // bias as texture is only sampled at the vertices of the patch
    vec3 min_pos = inPos[0];
    min_pos.z = (texture(height_map, inUV[0]).r - eps) * pc.height_scale;

    vec3 max_pos = inPos[0];
    max_pos.z = (texture(height_map, inUV[0]).r + eps) * pc.height_scale;
    
    for (int i=1; i < 4; i++) {
        vec3 pos = inPos[i];
        pos.z = texture(height_map, inUV[i]).r * pc.height_scale;

        min_pos = min(min_pos, pos);
        max_pos = max(max_pos, pos);
    }

    for (int i = 0; i < 6; i++) {
        bool outside = true;

        outside = outside && dot(planes[i], vec4(min_pos.x, min_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, min_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, max_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, min_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, max_pos.y, min_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(min_pos.x, max_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, min_pos.y, max_pos.z, 1)) < 0;
        outside = outside && dot(planes[i], vec4(max_pos.x, max_pos.y, max_pos.z, 1)) < 0;

        if (outside) {
            return true; // is culled
        }
    }

    return false;
}


float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float f_der_y(vec2 uv, vec2 dir, float eps) {
    // for dydu dir == vec2(1,0) for dydv dir == vec2(0,1)
    return (texture(height_map, uv + eps*dir).r - texture(height_map, uv - eps*dir).r) / (2*eps);
}

float s_der_y(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    
    return (
        texture(height_map, uv + eps1*2*dir1 + eps2*2*dir2).r
        - 2 * texture(height_map, uv).r
        + texture(height_map, uv - eps1*2*dir1 - eps2*2*dir2).r
    ) / (eps1*2 * eps2*2);
    

    //return (f_der_y(uv + eps2*dir2, dir1, eps1) - f_der_y(uv - eps2*dir2, dir1, eps1)) / (2*eps2);
}

const float smoothness_factor = 50.0;
float normalized_derivative(float der, float eps) {
    return clamp(map(abs(der),0,1.0/(smoothness_factor * 2*eps), 0, 1), 0, 1);
}

float normalized_s_der(vec2 uv, vec2 dir1, vec2 dir2, float eps1, float eps2) {
    float f_der1 = f_der_y(uv + eps2*dir2, dir1, eps1);
    float f_der2 = f_der_y(uv - eps2*dir2, dir1, eps1);

    return normalized_derivative((f_der1-f_der2) / (2*eps2), eps1*eps2);
}

// TODO: Set by uniform buffer object or extracted from projection matrix
const float zNear = 0.1;
const float zFar  = 100.0;
float linearize_depth(float d)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

float inout_bezier(float x) {
    return x * x * (3 - 2*x);
}

vec2 curvature(float K_uu, float K_vv, float K_uv) {
    // res.x mean curvature
    // res.y gaussian curvature

    float A = K_uu;
    float C = K_vv;
    float B = K_uv - (K_uu + K_vv) / 2;

    vec2 res = vec2(0);
    res.x = A + C;
    res.y = A*C - B * B;
    return res;
}

vec4 compute_tesselation_level() {
    // https://en.wikipedia.org/wiki/Curvature#Graph_of_a_function
    // Combining gaussian and mean curvature

    //vec2 texSize = textureSize(height_map,0);
    //vec2 epsilon = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    // 1 pixel distance sample is a bad estimate of derivatives / curvature at a patches vertex
    vec2 epsilon = vec2(inUV[2] - inUV[0]) * pc.texture_eps;

    vec4 res = vec4(0);

    for (int i = 0; i < 4; i++) {
        float huu = s_der_y(inUV[i], vec2(1,0), vec2(1,0), epsilon[0], epsilon[0]);
        float hvv = s_der_y(inUV[i], vec2(0,1), vec2(0,1), epsilon[1], epsilon[1]);
        float huv = s_der_y(inUV[i], vec2(1,0), vec2(0,1), epsilon[0], epsilon[1]);
        float hvu = s_der_y(inUV[i], vec2(0,1), vec2(1,0), epsilon[1], epsilon[0]);

        float hu = f_der_y(inUV[i], vec2(1,0), epsilon[0]);
        float hv = f_der_y(inUV[i], vec2(0,1), epsilon[1]);

        // definition taken from https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/adaptive-terrain-tessellation.pdf
        // mean curvature
        float H = (huu * (1 + hv*hv) - (huv+hvu)*hu*hv + hvv * (1 + hu*hu)) / (2*pow(1 + hu*hu + hv*hv, 1.5));
        // gaussian curvature
        float K = (huu + hvv - 0.5*(huv+hvu)*0.5*(huv+hvu)) / (1 + hu*hu + hv*hv);
        
        res[i] = abs(H);


        /*
        // approach 1 derivatives (bad) : sqrt(dydu*dydu + dydv*dydv)
        //res[i] = 0.01 * abs((ddydduu * ddyddvv - ddydduv*ddyddvu) / pow(1 + dydu*dydu + dydv * dydv, 2));
        float K_uu = ddydduu / pow(1 + dydu * dydu, 1.5);
        float K_vv = ddyddvv / pow(1 + dydv * dydv, 1.5);
        float K_uv = (ddydduu * ddydduu + ddyddvv * ddydduv + 2 * ddydduv) / 2 / pow(1 + pow(dydu + dydv, 2) / 2, 1.5);

        //vec2 c = curvature(K_uu, K_vv, K_uv);
        
        res[i] = abs(K_uu) + abs(K_vv);
        //res[i] = abs(K_uu);
        //res[i] = abs(c.y);
        */
    }

    return res;
}