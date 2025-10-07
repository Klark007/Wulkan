#pragma once

#include "vk_wrap/VKW_GraphicsPipeline.h"

// forward decleration
template<typename T, size_t N>
class RenderPass;

// Object that is created per Shape of that RenderPass, needs to be deleted by the mesh
template<typename T, size_t N>
class MaterialInstance : public VKW_Object {
public:
	MaterialInstance() = default;

	template<size_t M>
	void init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<T,M>& render_pass, const std::array<VKW_DescriptorSetLayout, N>& descriptor_layouts, const std::array<uint32_t, N>& set_slots, const std::string& material_name);

	void del() override;
private:
	std::array<std::array<VKW_DescriptorSet, N>, MAX_FRAMES_IN_FLIGHT> m_descriptor_sets;
	std::array<uint32_t, N> m_set_slots;
	VKW_PushConstant<T>* m_push_constant;
	VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
public:
	// can be bound also when using a different RenderPass is currently bound (i.e. use one Instance for both the normal shading of terrain as well as a depth only pass)
	// as long as same Pipeline Layout ?
	void bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, const T& push_val);
	VKW_DescriptorSet& get_descriptor_set(size_t frame_idx, size_t set_idx) { return m_descriptor_sets[frame_idx][set_idx]; };
};

template<typename T, size_t N>
template<size_t M>
inline void MaterialInstance<T, N>::init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<T, M>& render_pass, const std::array<VKW_DescriptorSetLayout, N>& descriptor_layouts, const std::array<uint32_t, N>& set_slots, const std::string& material_name)
{
	m_set_slots = set_slots;

	for (auto& per_frame_sets : m_descriptor_sets) {
		for (int i = 0; i < N; i++) {
			per_frame_sets[i].init(
				&device,
				&descriptor_pool,
				descriptor_layouts[i],
				fmt::format("{} Desc Set", material_name)
			);
		}
	}

	m_push_constant = &render_pass.m_push_constant;
	m_pipeline_layout = render_pass.m_pipeline.get_layout();
}

template<typename T, size_t N>
inline void MaterialInstance<T, N>::bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, const T& push_val)
{
	// TODO make into one bind call
	for (size_t i = 0; i < N; i++) {
		m_descriptor_sets[current_frame][i].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, m_set_slots[i]);
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
