#pragma once

#include "../Path.h"

#include "VKW_Object.h"
#include "VKW_Device.h"
#include "VKW_SpecializationConstants.h"

class VKW_Shader : public VKW_Object
{
public:
	VKW_Shader() = default;
	// assumes a *_d{Type}.spv and a *_d{Type}.spv with Types such as frag, vert, comp etc
	void init(const VKW_Device* device, const VKW_Path& path, VkShaderStageFlagBits stage, const std::string& obj_name, const std::string& entry_func = "main", const VkSpecializationInfo* spezialisation_const = NULL);
	void del() override;
private:
	const VKW_Device* device = nullptr;
	std::string name;
	std::string entry;

	VkShaderModule module = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo shader_stage_info{};
public:
	const VkPipelineShaderStageCreateInfo& get_info() const { return shader_stage_info; };
};

