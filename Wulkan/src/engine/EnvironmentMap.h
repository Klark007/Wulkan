#pragma once

#include "common.h"
#include "Shape.h"
#include "Mesh.h"
#include "Texture.h"
#include "vk_wrap/VKW_Sampler.h"

#include "vk_wrap/VKW_GraphicsPipeline.h"

struct EnvironmentMapPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
};

class SharedEnvironmentData : public VKW_Object {
public:
	SharedEnvironmentData() = default;
	// initializes data
	void init(const VKW_Device* device);
	void del() override;
private:
	VKW_DescriptorSetLayout descriptor_set_layout;
	VKW_PushConstant<EnvironmentMapPushConstants> push_constant;
public:
	const VKW_DescriptorSetLayout& get_descriptor_set_layout() const { return descriptor_set_layout; };

	inline std::vector<VkPushConstantRange> get_push_consts_range() const {
		return { push_constant.get_range() };
	}

	inline VKW_PushConstant<EnvironmentMapPushConstants>& get_pc() { return push_constant; };
};

class EnvironmentMap : public Shape
{
public:
	EnvironmentMap() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, SharedEnvironmentData* shared_environment_data, const std::string& path);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers, const VKW_Sampler& texture_sampler);
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
private:
	Texture cube_map;
	Mesh mesh; // incorrect normals and uv
	VKW_Sampler sampler;

	std::array<VKW_DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
	SharedEnvironmentData* shared_data;
public:
	static VKW_GraphicsPipeline create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedEnvironmentData& shared_environment_data);
};

inline void EnvironmentMap::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
	descriptor_sets.at(current_frame).bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout());

	shared_data->get_pc().update({
		mesh.get_vertex_address()
	});
	shared_data->get_pc().push(command_buffer, pipeline.get_layout());

	mesh.draw(command_buffer, current_frame, pipeline);
}

inline VKW_GraphicsPipeline EnvironmentMap::create_pipeline(const VKW_Device* device, Texture& color_rt, Texture& depth_rt, const SharedEnvironmentData& shared_environment_data)
{
	VKW_Shader vert_shader{};
	vert_shader.init(device, "shaders/environment_maps/environment_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "Environment map vertex shader");

	VKW_Shader frag_shader{};
	frag_shader.init(device, "shaders/environment_maps/environment_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "Environment map fragment shader");

	VKW_GraphicsPipeline graphics_pipeline{};

	graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	graphics_pipeline.set_culling_mode();
	
	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });
	graphics_pipeline.add_descriptor_sets({ shared_environment_data.get_descriptor_set_layout() });

	graphics_pipeline.add_push_constants(shared_environment_data.get_push_consts_range());

	graphics_pipeline.set_color_attachment_format(color_rt.get_format());
	graphics_pipeline.set_depth_attachment_format(depth_rt.get_format());

	graphics_pipeline.init(device, "Environment graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	return graphics_pipeline;
}

