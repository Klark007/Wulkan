#pragma once

#include "VKW_Instance.h"
#include "../vk_types.h"

class VKW_Surface {
public:
	VKW_Surface(struct GLFWwindow* window, std::shared_ptr<VKW_Instance> instance);
	~VKW_Surface();
private:
	std::shared_ptr<VKW_Instance> instance;
	VkSurfaceKHR surface;
public:
	inline VkSurfaceKHR get_surface() const { return surface; };
};