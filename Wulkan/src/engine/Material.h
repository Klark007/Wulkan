#pragma once

#include <format>
#include "vk_wrap/VKW_GraphicsPipeline.h"

// forward decleration
template<typename T, size_t N>
class RenderPass;

// Object that is created per Shape of that RenderPass, needs to be deleted by the mesh
template<typename T, size_t N>
class MaterialInstance : public VKW_Object {
public:
	MaterialInstance() = default;

	// Important layouts array order matters
	// It corresponds to the order in which the multiple sets will be bound (set slot)
	void init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<T,N>& render_pass, const std::string& material_name);

	void del() override;
private:
	std::array<std::array<VKW_DescriptorSet, N>, MAX_FRAMES_IN_FLIGHT> m_descriptor_sets;
	VKW_PushConstant<T>* m_push_constant;
	VkPipelineLayout m_pipeline_layout;
public:
	// can be bound also when using a different RenderPass is currently bound (i.e. use one Instance for both the normal shading of terrain as well as a depth only pass)
	// as long as same Pipeline Layout ?
	void bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, T push_val);
	VKW_DescriptorSet& get_descriptor_set(size_t frame_idx, size_t set_idx) { return m_descriptor_sets[frame_idx][set_idx]; };
};

template<typename T, size_t N>
inline void MaterialInstance<T, N>::init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<T, N>& render_pass, const std::string& material_name)
{
	for (auto& per_frame_sets : m_descriptor_sets) {
		for (int i = 0; i < N; i++) {
			per_frame_sets[i].init(&device, &descriptor_pool, render_pass.m_layouts[i], std::format("{} Desc Set", material_name));
		}
	}

	m_push_constant = &render_pass.m_push_constant;
	m_pipeline_layout = render_pass.m_pipeline.get_layout();
}

template<typename T, size_t N>
inline void MaterialInstance<T, N>::bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, T push_val)
{
	// TODO make into one bind call
	for (int i = 0; i < N; i++) {
		m_descriptor_sets[current_frame][i].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, i);
	}
	m_push_constant->update(push_val);
	m_push_constant->push(cmd, m_pipeline_layout);
}

template<typename T, size_t N>
inline void MaterialInstance<T, N>::del()
{
	for (auto& per_frame_sets : m_descriptor_sets) {
		for (VKW_DescriptorSet& set : per_frame_sets) {
			set.del();
		}
	}
}


