#pragma once

#include "../common.h"
#include "VKW_Object.h"
#include "VKW_Instance.h"


class VKW_Surface : public VKW_Object {
public:
	VKW_Surface() = default;
	void init(struct GLFWwindow* window, const VKW_Instance* vkw_instance);
	void del() override;
private:
	const VKW_Instance* instance;
	VkSurfaceKHR surface;
public:
	inline VkSurfaceKHR get_surface() const { return surface; };
	inline operator VkSurfaceKHR() const { return surface; };
};