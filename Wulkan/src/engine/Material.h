#pragma once

#include <format>
#include "vk_wrap/VKW_GraphicsPipeline.h"

// forward decleration
template<typename T, size_t N>
class MaterialType;

// Object that is created per Shape of that MaterialType, needs to be deleted by the mesh
template<typename T, size_t N>
class MaterialInstance : public VKW_Object {
public:
	MaterialInstance() = default;

	// Important layouts array order matters
	// It corresponds to the order in which the multiple sets will be bound (set slot)
	void init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, std::array<VKW_DescriptorSetLayout, N> layouts, MaterialType<T, N>* type, VkPipelineLayout pipeline_layout, std::string name);

	void del() override;
private:
	std::array<std::array<VKW_DescriptorSet, N>, MAX_FRAMES_IN_FLIGHT> m_descriptor_sets;
	MaterialType<T, N>* m_type;
	VkPipelineLayout m_pipeline_layout;
public:
	void bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, T push_val);
	VKW_DescriptorSet& get_descriptor_set(size_t frame_idx, size_t set_idx) { return m_descriptor_sets[frame_idx][set_idx]; };
};

template<typename T, size_t N>
class MaterialType : public VKW_Object
{
public:
	MaterialType() = default;

	// init with initalized pipeline
	// Pipeline is owned by MaterialType, layouts not
	void init(const VKW_GraphicsPipeline& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant, const std::string& mat_name);
	void del() override;

	// binds current material type, expects to be in active command buffer cmd
	void begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, Texture& color_rt, Texture& depth_rt, bool do_clear_color=false, VkClearColorValue clear_color_value= {{1,0,1,1}}, bool do_clear_depth=false, float clear_depth_value=0);
	void end(const VKW_CommandBuffer& cmd);
private:
	VKW_GraphicsPipeline m_pipeline;
	std::array<VKW_DescriptorSetLayout, N> m_layouts;
	VKW_PushConstant<T> m_push_constant;

	std::string m_name;
	mutable uint64_t num_instances;
public:
	MaterialInstance<T, N> create_material_instance(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool);
	VKW_PushConstant<T>& get_push_constant() { return m_push_constant; };
};

template<typename T, size_t N>
void MaterialType<T, N>::init(const VKW_GraphicsPipeline& pipeline, const std::array<VKW_DescriptorSetLayout, N>& layouts, const VKW_PushConstant<T>& push_constant, const std::string& mat_name)
{
	m_pipeline = pipeline;
	m_layouts = layouts;
	m_push_constant = push_constant;
	m_name = mat_name;

	num_instances = 0;
}

template<typename T, size_t N>
inline void MaterialType<T, N>::del()
{
	m_pipeline.del();
}

template<typename T, size_t N>
inline void MaterialType<T, N>::begin(const VKW_CommandBuffer& cmd, VkExtent2D extent, Texture& color_rt, Texture& depth_rt, bool do_clear_color, VkClearColorValue clear_color_value, bool do_clear_depth, float clear_depth_value)
{
	m_pipeline.set_render_size(extent);

	m_pipeline.set_color_attachment(
		color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
		do_clear_color,
		clear_color_value
	);

	m_pipeline.set_depth_attachment(
		depth_rt.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
		do_clear_depth,
		clear_depth_value
	);

	m_pipeline.begin_rendering(cmd);

	m_pipeline.bind(cmd);

	m_pipeline.set_dynamic_state(cmd);
}

template<typename T, size_t N>
inline void MaterialType<T, N>::end(const VKW_CommandBuffer& cmd)
{
	m_pipeline.end_rendering(cmd);
}

template<typename T, size_t N>
inline MaterialInstance<T, N> MaterialType<T, N>::create_material_instance(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool)
{
	MaterialInstance<T, N> instance{};
	instance.init(
		device, descriptor_pool, m_layouts, this, m_pipeline.get_layout(), std::format("{} [{}]", m_name, num_instances)
	);
	num_instances++;

	return instance;
}

template<typename T, size_t N>
inline void MaterialInstance<T, N>::init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, std::array<VKW_DescriptorSetLayout, N> layouts, MaterialType<T, N>* type, VkPipelineLayout pipeline_layout, std::string name)
{
	for (auto& per_frame_sets : m_descriptor_sets) {
		for (int i = 0; i < N; i++) {
			per_frame_sets[i].init(&device, &descriptor_pool, layouts[i], name + " Desc Set");
		}
	}

	m_type = type;
	m_pipeline_layout = pipeline_layout;
}

template<typename T, size_t N>
inline void MaterialInstance<T, N>::bind(const VKW_CommandBuffer& cmd, uint32_t current_frame, T push_val)
{
	for (int i = 0; i < N; i++) {
		m_descriptor_sets[current_frame][i].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout);
	}
	m_type->get_push_constant().update(push_val);
	m_type->get_push_constant().push(cmd, m_pipeline_layout);
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


