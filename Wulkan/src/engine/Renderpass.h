#pragma once
#include "Material.h"

template<typename T, size_t N>
class RenderPass : public VKW_Object
{
public:
	RenderPass() = default;

	// friend for any S,M (it seems that its not possible to fix T and keep M varying)
	template<typename S, size_t M> 
	friend class MaterialInstance;

	// init with initalized pipeline
	// Pipeline is owned by RenderPass, layouts not
	void init(VKW_GraphicsPipeline&& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant);
	virtual void del() override;

	// binds current render pass, expects to be in active command buffer cmd
	// pass in VK_NULL_HANDLE in color_rt or depth_rt to not set them in the pipeline
	void begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, VkImageView color_rt, VkImageView depth_rt, VkImageView color_rt_resolve = VK_NULL_HANDLE, VkImageView depth_rt_resolve = VK_NULL_HANDLE, bool do_clear_color = false, VkClearColorValue clear_color_value = { {1,0,1,1} }, bool do_clear_depth = false, float clear_depth_value = 0, float const_depth_bias = 0, float slope_depth_bias = 0);
	void end(const VKW_CommandBuffer& cmd);
	
protected:
	VKW_GraphicsPipeline m_pipeline;
	VKW_PushConstant<T> m_push_constant;
public:
	VkPipelineLayout get_pipeline_layout() const { return m_pipeline.get_layout(); };
};

template<typename T, size_t N>
void RenderPass<T, N>::init(VKW_GraphicsPipeline&& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant)
{
	m_pipeline = pipeline;
	//m_layouts = layouts;
	m_push_constant = push_constant;
}

template<typename T, size_t N>
inline void RenderPass<T, N>::del()
{
	m_pipeline.del();
}

template<typename T, size_t N>
inline void RenderPass<T, N>::begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, VkImageView color_rt, VkImageView depth_rt, VkImageView color_rt_resolve, VkImageView depth_rt_resolve, bool do_clear_color, VkClearColorValue clear_color_value, bool do_clear_depth, float clear_depth_value, float const_depth_bias, float slope_depth_bias)
{
	m_pipeline.set_render_size(extent);

	// TODO: does this need to happen per frame?
	if (color_rt != VK_NULL_HANDLE) {
		m_pipeline.set_color_attachment(
			color_rt,
			do_clear_color,
			clear_color_value,
			color_rt_resolve,
			(color_rt_resolve != VK_NULL_HANDLE) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE
		);
	}

	if (depth_rt != VK_NULL_HANDLE) {
		m_pipeline.set_depth_attachment(
			depth_rt,
			do_clear_depth,
			clear_depth_value,
			depth_rt_resolve,
			(depth_rt_resolve != VK_NULL_HANDLE) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE
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