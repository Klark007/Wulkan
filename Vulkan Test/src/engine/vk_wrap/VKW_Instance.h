#pragma once

#include "../vk_types.h"

class VKW_Instance
{
public:
	VKW_Instance(const std::string& app_name, std::vector<const char*> instance_extensions, std::vector<const char*> instance_layers);
	~VKW_Instance();
private:
	vkb::Instance vkb_instance;
public:
	inline vkb::Instance get_vkb_instance() const { return vkb_instance; };
	inline VkInstance get_instance() const { return vkb_instance.instance; };
};

