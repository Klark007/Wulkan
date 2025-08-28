#pragma once

#include "common.h"
#include "Shape.h"
#include "Mesh.h"
#include "Texture.h"
#include "vk_wrap/VKW_Sampler.h"

#include "vk_wrap/VKW_GraphicsPipeline.h"
#include "Renderpass.h"

struct EnvironmentMapPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
};

class EnvironmentMap : public Shape
{
public:
	EnvironmentMap() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, const VKW_DescriptorPool& descriptor_pool, RenderPass<EnvironmentMapPushConstants, 2>& render_pass, const std::string& path);
	void set_descriptor_bindings(const std::array<VKW_Buffer, MAX_FRAMES_IN_FLIGHT>& uniform_buffers, const VKW_Sampler& texture_sampler);
	void del() override;

	static RenderPass<EnvironmentMapPushConstants, 2> create_render_pass(const VKW_Device* device, const std::array<VKW_DescriptorSetLayout, 2>& layouts, Texture& color_rt, Texture& depth_rt);
	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:
	Texture cube_map;
	Mesh mesh; // incorrect normals and uv
	VKW_Sampler sampler;

	MaterialInstance< EnvironmentMapPushConstants, 2> material;
};

inline void EnvironmentMap::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	material.bind(
		command_buffer,
		current_frame,
		{ mesh.get_vertex_address() }
	);

	mesh.draw(command_buffer, current_frame);
}

