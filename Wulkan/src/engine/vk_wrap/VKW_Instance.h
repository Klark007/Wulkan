#pragma once

#include "VKW_Object.h"

class VKW_Instance : public VKW_Object {
public:
	VKW_Instance() = default;
	void init(const std::string& app_name, std::vector<const char*> instance_extensions, std::vector<const char*> instance_layers);
	void del() override;
private:
	vkb::Instance vkb_instance;
public:
	inline const vkb::Instance& get_vkb_instance() const { return vkb_instance; };
	inline VkInstance get_instance() const { return vkb_instance.instance; };
	inline operator VkInstance() const { return vkb_instance.instance; };
};

