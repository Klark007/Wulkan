#pragma once
#include "Material.h"

template<typename T, size_t N>
class RenderPass : public VKW_Object
{
public:
	RenderPass();
	friend MaterialInstance;

	// init with initalized pipeline
	// Pipeline is owned by RenderPass, layouts not
	void init(const VKW_GraphicsPipeline& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant);
	void del() override;

	// binds current render pass, expects to be in active command buffer cmd
	// pass in VK_NULL_HANDLE in color_rt or depth_rt to not set them in the pipeline
	void begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, VkImageView color_rt, VkImageView depth_rt, bool do_clear_color = false, VkClearColorValue clear_color_value = { {1,0,1,1} }, bool do_clear_depth = false, float clear_depth_value = 0, float const_depth_bias = 0, float slope_depth_bias = 0);
	void end(const VKW_CommandBuffer& cmd);
	
private:
	VKW_GraphicsPipeline m_pipeline;
	std::array<VKW_DescriptorSetLayout, N> m_layouts;
	VKW_PushConstant<T> m_push_constant;
public:
	VkPipelineLayout get_pipeline_layout() const { return m_pipeline.get_layout(); };
};


template<typename T, size_t N>
inline RenderPass<T, N>::RenderPass()
	: m_pipeline{}, m_layouts{}, m_push_constant{}
{}

template<typename T, size_t N>
void RenderPass<T, N>::init(const VKW_GraphicsPipeline& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant)
{
	m_pipeline = pipeline;
	m_layouts = layouts;
	m_push_constant = push_constant;
}

template<typename T, size_t N>
inline void RenderPass<T, N>::del()
{
	m_pipeline.del();
}

template<typename T, size_t N>
inline void RenderPass<T, N>::begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, VkImageView color_rt, VkImageView depth_rt, bool do_clear_color, VkClearColorValue clear_color_value, bool do_clear_depth, float clear_depth_value, float const_depth_bias, float slope_depth_bias)
{
	m_pipeline.set_render_size(extent);

	if (color_rt != VK_NULL_HANDLE) {
		m_pipeline.set_color_attachment(
			color_rt,
			do_clear_color,
			clear_color_value
		);
	}

	if (depth_rt != VK_NULL_HANDLE) {
		m_pipeline.set_depth_attachment(
			depth_rt,
			do_clear_depth,
			clear_depth_value
		);
	}

	m_pipeline.begin_rendering(cmd);

	m_pipeline.bind(cmd);

	m_pipeline.set_dynamic_state(cmd);

	if (const_depth_bias != 0 || slope_depth_bias != 0)
		m_pipeline.set_depth_bias(const_depth_bias, slope_depth_bias, cmd);
}

template<typename T, size_t N>
inline void RenderPass<T, N>::end(const VKW_CommandBuffer& cmd)
{
	m_pipeline.end_rendering(cmd);
}