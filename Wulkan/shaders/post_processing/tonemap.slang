struct Vertex {
	float3 position;
	float uv_x;
	float3 normal;
	float uv_y;
	float4 color;
};

struct Instance {
    float3 position;
}

[[vk::push_constant]]
cbuffer PushConstant
{
    uint64_t vertex_address;
    int tone_mapper_mode;
    float l_white_point; // luminance
} pc;

struct VOut
{
    float4 pos : SV_POSITION; // corresponds to gl_Position
    float2 uv;
}

// SV_VertexID computes gl_VertexIndex - gl_BaseVertex
// see https://registry.khronos.org/OpenGL/extensions/ARB/ARB_shader_draw_parameters.txt
[shader("vertex")] 
VOut vertexMain(uint vertexID : SV_VulkanVertexID, uint instanceIndex : SV_VulkanInstanceID)
{
    Vertex *vertices =
        (Vertex *)pc.vertex_address;
    Vertex v = vertices[vertexID];

    float2 d_pos = float2(v.position.x*2 - 1, v.position.y*2 - 1); // NDC between -1 and 1

    VOut output;
    output.pos = float4(d_pos.x, d_pos.y, 0.0, 1.0);
    output.uv = float2(v.uv_x, v.uv_y);
    return output;
}


// first binding then set
[[vk::binding(0, 1)]]
Sampler2D frame;

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let color = frame.Sample(input.uv);

    if (any(isinf(color))) {
        return color;
    }

    switch (pc.tone_mapper_mode) {
        case 0: // None
            return color;
        case 1: // Rheinhard
            return float4(rheinhard(color.rgb), 1);
        case 2: // extended rheinhard
            return float4(ext_rheinhard(color.rgb), 1);
        case 3: // Uncharted filmic
            return float4(uncharted_filmic(color.rgb), 1);
        case 4: // ACES
            return float4(aces_approx(color.rgb), 1);
        case 5: // AgX
            float3 agx_color = agx(color.rgb);
            agx_color = agxLook(agx_color);
            agx_color = agxEotf(agx_color);

            return float4(agx_color, 1);
        default:
            return float4(1, 0, 1, 1);
    }
}

float luminance(float3 c) {
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float3 rheinhard(float3 c) {
    float old_l = luminance(c);
    float new_l = old_l / (1 + old_l);
    return c * new_l / old_l;
}

float3 ext_rheinhard(float3 c) {
    float old_l = luminance(c);
    float new_l = (old_l * (1 + (old_l / (pc.l_white_point * pc.l_white_point)))) / (1 + old_l);
    return c * new_l / old_l;
}

float3 uncharted2_tonemap_partial(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Look like bad paramters in the scene
float3 uncharted_filmic(float3 c)
{
    float exposure_bias = 2.0;
    float3 curr = uncharted2_tonemap_partial(c * exposure_bias);

    float3 W = float3(11.2);
    float3 white_scale = float3(1.0) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

float3 aces_approx(float3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
}

// Based on https://www.shadertoy.com/view/cd3XWr
// 0: Default, 1: Golden, 2: Punchy
#define AGX_LOOK 2

// Mean error^2: 3.6705141e-06
float3 agxDefaultContrastApprox(float3 x) {
    float3 x2 = x * x;
    float3 x4 = x2 * x2;

    return +15.5 * x4 * x2
         - 40.14 * x4 * x
         + 31.96 * x4
         - 6.868 * x2 * x
         + 0.4298 * x2
         + 0.1191 * x
         - 0.00232;
}

float3 agx(float3 val) {
    // Slang needs transposed
    const float3x3 agx_mat = float3x3(
    0.842479062253094, 0.0784335999999992, 0.0792237451477643,
    0.0423282422610123, 0.878468636469772, 0.0791661274605434,
    0.0423756549057051, 0.0784336, 0.879142973793104);

    const float min_ev = -12.47393f;
    const float max_ev = 4.026069f;

    // Input transform
    val = mul(agx_mat, val);

    // Log2 space encoding
    val = clamp(log2(val), min_ev, max_ev);
    val = (val - min_ev) / (max_ev - min_ev);

    // Apply sigmoid function approximation
    val = agxDefaultContrastApprox(val);

    return val;
}

float3 agxEotf(float3 val) {
    // Slang needs transposed
    const float3x3 agx_mat_inv = float3x3(
    1.19687900512017, -0.0980208811401368, -0.0990297440797205,
    -0.0528968517574562, 1.15190312990417, -0.0989611768448433,
    -0.0529716355144438, -0.0980434501171241, 1.15107367264116);

    // Undo input transform
    val = mul(agx_mat_inv, val);

    // sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
    val = pow(val, float3(2.2));

    return val;
}

float3 agxLook(float3 val) {
    // Default
    float3 offset = float3(0.0);
    float3 slope = float3(1.0);
    float3 power = float3(1.0);
    float sat = 1.0;

#if AGX_LOOK == 1
    // Golden
    slope = float3(1.0, 0.9, 0.5);
    power = float3(0.8);
    sat = 0.8;
#elif AGX_LOOK == 2
    // Punchy
    slope = float3(1.0);
    power = float3(1.35, 1.35, 1.35);
    sat = 1.4;
#endif

    // ASC CDL
    val = pow(val * slope + offset, power);

    const float3 lw = float3(0.2126, 0.7152, 0.0722);
    float luma = dot(val, lw);

    return luma + sat * (val - luma);
}