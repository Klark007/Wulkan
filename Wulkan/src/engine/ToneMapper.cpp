#include "common.h"
#include "ToneMapper.h"

void ToneMapper::init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, const std::array<VKW_DescriptorSetLayout, 2>& layouts, VkFormat color_attachment_format)
{
	// hard coded view plane
	const std::vector<Vertex> vertices = {
		{glm::vec3(0.0,0.0,0.0), 0.0, {}, 0.0, {}},
		{glm::vec3(0.0,1.0,0.0), 0.0, {}, 1.0, {}},
		{glm::vec3(1.0,0.0,0.0), 1.0, {}, 0.0, {}},
		{glm::vec3(1.0,1.0,0.0), 1.0, {}, 1.0, {}}
	};
	const std::vector<uint32_t> indices = { 0,1,2,1,3,2 };

	view_plane.init(device, transfer_pool, vertices, indices);

	// push constants
	VKW_PushConstant<ToneMapperPushConstants> push_constant{};
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	// graphics pipeline	
	// defaults: no depth test / write, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	VKW_GraphicsPipeline graphics_pipeline{};

	VKW_Shader vert_shader{};
	vert_shader.init(&device, "shaders/post_processing/tone_map_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "ToneMapper vertex shader");

	VKW_Shader frag_shader{};
	//frag_shader.init(&device, "shaders/post_processing/tone_map_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "ToneMapper fragment shader");
	frag_shader.init(&device, "shaders/post_processing/tonemap_slang.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "ToneMapper fragment shader", "fragmentMain");

	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });
	
	graphics_pipeline.add_descriptor_sets(layouts);
	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	graphics_pipeline.set_color_attachment_format(color_attachment_format);

	graphics_pipeline.init(&device, "Tone Mapping graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	// super init
	RenderPass::init(std::move(graphics_pipeline), layouts, push_constant);

	material.init(device, descriptor_pool, *this, { descriptor_set_layout }, { 1 }, "Tone Mapper Material");
}

void ToneMapper::set_descriptor_bindings(const std::array<VkImageView, MAX_FRAMES_IN_FLIGHT>& views, const VKW_Sampler& texture_sampler)
{
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set1 = material.get_descriptor_set(i, 0);
		set1.update(0, views[i], texture_sampler);
	}
}

void ToneMapper::del()
{
	material.del();
	RenderPass::del();
	view_plane.del();
}

VKW_DescriptorSetLayout ToneMapper::create_descriptor_set_layout(const VKW_Device& device)
{
	descriptor_set_layout = VKW_DescriptorSetLayout{};

	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);
	descriptor_set_layout.init(&device, "Tone Mapper Desc Layout");

	return descriptor_set_layout;
}
