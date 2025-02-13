#pragma once

#include "../vk_types.h"
#include "VKW_Object.h"

#include "VKW_Device.h"

class VKW_Shader : public VKW_Object
{
public:
	VKW_Shader() = default;
	void init(const VKW_Device* device, const std::string& path, VkShaderStageFlagBits stage, const std::string& obj_name, const std::string& entry_func = "main");
	void del() override;
private:
	const VKW_Device* device;
	std::string name;
	std::string entry;

	VkShaderModule module;
	VkPipelineShaderStageCreateInfo shader_stage_info;
public:
	VkPipelineShaderStageCreateInfo get_info() const { return shader_stage_info; };
};

