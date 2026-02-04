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

/*
[[vk::push_constant]]
cbuffer PushConstant
{
    float4x4 model;
    float4x4 inv_model;
    Vertex[] * vertex_buffer; // Or  ConstBufferPointer<Vertex2[]>
    Instance[]* instance_buffer;
    int cascade_idx;
    int lod_level;
} pc;
*/

[[vk::push_constant]]
cbuffer PushConstant
{
    //Vertex[] * vertex_buffer; // Or  ConstBufferPointer<Vertex2[]>
    uint64_t vertex_address;
    int tone_mapper_mode;
    float l_white_point; // luminance
} pc;

struct VOut
{
    float2 uv;
}

// SV_VertexID computes gl_VertexIndex - gl_BaseVertex
// see https://registry.khronos.org/OpenGL/extensions/ARB/ARB_shader_draw_parameters.txt
[shader("vertex")] 
VOut vertexMain(uint vertexID : SV_VulkanVertexID, uint instanceIndex : SV_VulkanInstanceID)
{
    // Vertex v = (*pc.vertex_buffer)[vertexID]; // or pc.vertex_buffer.get()[vertexID]

    Vertex *vertices =
        (Vertex *)pc.vertex_address;
    Vertex v = vertices[vertexID];

    float2 d_pos = float2(v.position.x*2 - 1, v.position.y*2 - 1); // NDC between -1 and 1
    
    VOut output;
    output.uv = d_pos;
    return output;
}


// first binding then set
[[vk::binding(0, 1)]]
Sampler2D frame;

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let color = frame.Sample(input.uv);
    return color;
}