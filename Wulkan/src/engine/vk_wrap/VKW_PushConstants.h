#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"
#include "VKW_CommandBuffer.h"

// represents push constants, currently just one VKW_PushConstants in a pipeline, so no offset
template <typename T> class VKW_PushConstants
{
public:
	VKW_PushConstants() = default;

	void init(VkShaderStageFlags shader_stages, uint32_t offset = 0);
private:
	VkPushConstantRange range;
	T data;
public:
	inline void update(T t) { data = t; };
	// pushes the set data, assumes to be recording commands into the above command buffer
	inline void push(const VKW_CommandBuffer& command_buffer, VkPipelineLayout layout) const;

	VkPushConstantRange get_range() const { return range; };
};

template<typename T>
inline void VKW_PushConstants<T>::init(VkShaderStageFlags shader_stages, uint32_t offset)
{
	range.stageFlags = shader_stages;
	range.offset = offset;
	range.size = sizeof(T);
}

template<typename T>
inline void VKW_PushConstants<T>::push(const VKW_CommandBuffer& command_buffer, VkPipelineLayout layout) const
{
	vkCmdPushConstants(command_buffer, layout, range.stageFlags, range.offset, range.size, &data);
}
