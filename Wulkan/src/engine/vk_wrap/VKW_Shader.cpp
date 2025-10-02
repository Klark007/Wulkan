#include "VKW_Shader.h"

#include <fstream>

#include <iostream>
void VKW_Shader::init(const VKW_Device* vkw_device, const VKW_Path& path, VkShaderStageFlagBits stage, const std::string& obj_name, const std::string& entry_func, const VkSpecializationInfo* spezialisation_const)
{
	device = vkw_device;
	name = obj_name;

#ifdef NDEBUG
	// open file and read compiled shader into it
	// start at end of file to compute buffer size
	std::ifstream file(path, std::ios::ate | std::ios::binary);
#else
	// replace *_{Type}.spv with *_d{Type}.spv
	std::string path_str = path.string();
	size_t last_underscore = path_str.rfind("_");
	VKW_Path actual_path = path_str.substr(0, last_underscore+1) + "d" + path_str.substr(last_underscore + 1);

	std::ifstream file(actual_path, std::ios::ate | std::ios::binary);
#endif

	if (!file.is_open()) {
		throw IOException(fmt::format("Failed to open shader file at {}", path), __FILE__, __LINE__);
	}

	// look location of cursor in file
	size_t file_size = static_cast<size_t>(file.tellg());

	// reset cursor and read data into buffer
	std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), file_size);

	file.close();


	// create shader module
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pCode = buffer.data();
	create_info.codeSize = file_size; // filesize is in bytes

	VK_CHECK_ET(vkCreateShaderModule(*device, &create_info, VK_NULL_HANDLE, &module), RuntimeException, fmt::format("Failed to create shader module ({})", name));
	device->name_object((uint64_t)module, VK_OBJECT_TYPE_SHADER_MODULE, name);

	// set up shader stage info
	entry = entry_func;
	shader_stage_info = {};
	shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_info.stage = stage;
	shader_stage_info.module = module;
	shader_stage_info.pName = entry.c_str();
	shader_stage_info.pSpecializationInfo = spezialisation_const;
}

void VKW_Shader::del()
{
	VK_DESTROY(module, vkDestroyShaderModule, *device, module);
}
