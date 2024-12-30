#pragma once

#include "../vk_types.h"
#include "VKW_Object.h"


#include "VKW_Device.h"

class VKW_Swapchain : public VKW_Object
{
public:
	VKW_Swapchain() = default;
	void init(struct GLFWwindow* window, const VKW_Device& device);
	void del() override;

	// TODO: add resizing with using the old swapchain
private:
	VkSwapchainKHR swapchain;
	vkb::Swapchain vkb_swapchain;
	std::vector<VkImageView> image_views;

	std::vector<VkImage> get_images();
	std::vector<VkImageView> get_image_views();
public:
	inline VkSwapchainKHR get_swapchain() const { return swapchain; };
	inline operator VkSwapchainKHR() const { return swapchain; };

	inline size_t size() const { return vkb_swapchain.image_count; };
	inline const VkImageView& at(size_t i) const { return image_views.at(i); };

	inline VkExtent2D get_extent() const { return vkb_swapchain.extent; };
	inline VkFormat get_format() const { return vkb_swapchain.image_format; };
};

