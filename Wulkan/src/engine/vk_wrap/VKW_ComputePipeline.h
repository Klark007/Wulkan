#pragma once

#include "../vk_types.h"
#include "VKW_Object.h"

#include "VKW_Device.h"
#include "VKW_Shader.h"
#include "VKW_DescriptorSet.h"
#include "VKW_PushConstants.h"

class VKW_ComputePipeline : public VKW_Object {
public:
	VKW_ComputePipeline() = default;
	void init(const VKW_Device* vkw_device, const std::string& shader_path, const std::string& obj_name);
	void del() override;
private:
	const VKW_Device* device;
	std::string name;
	VkPipeline compute_pipeline;
	VkPipelineLayout layout;

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
	std::vector<VkPushConstantRange> push_consts_range;
public:
	inline void bind(const VKW_CommandBuffer& cmd) const { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline); };
	inline void dispatch(const VKW_CommandBuffer& cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) const;


	// BEGIN SET BEFORE INIT
	// adds Descriptorset to the pipeline
	void add_descriptor_sets(const std::vector<VKW_DescriptorSetLayout>& layouts);

	// adds a pushconstant to the pipeline
	template <class T>
	inline void add_push_constant(VKW_PushConstant<T> push_const);
	void add_push_constants(const std::vector<VkPushConstantRange>& ranges);
	
	// END SET BEFORE INIT

	inline operator VkPipeline() const { return compute_pipeline; };
	inline VkPipeline get_pipeline() const { return compute_pipeline; };
	inline VkPipelineLayout get_layout() const { return layout; };
};

inline void VKW_ComputePipeline::dispatch(const VKW_CommandBuffer& cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) const
{
	vkCmdDispatch(cmd, group_count_x, group_count_y, group_count_z);
}

template<class T>
inline void VKW_ComputePipeline::add_push_constant(VKW_PushConstant<T> push_const)
{
	push_consts_range.push_back(push_const.get_range());
}