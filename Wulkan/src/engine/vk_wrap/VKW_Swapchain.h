#pragma once

#include "VKW_Object.h"


#include "VKW_Device.h"
#include "VKW_Queue.h"

class VKW_Swapchain : public VKW_Object
{
public:
	VKW_Swapchain() = default;
	void init(struct GLFWwindow* window, const VKW_Device& device, const VKW_Queue* present_queue, const std::string& obj_name, VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
	void del() override;

	void recreate(struct GLFWwindow* window, const VKW_Device& device);

	// hand image back to swapchain, if return false then swapchain (and the images we render into) needs to be recreated 
	bool present(const std::vector<VkSemaphore>& wait_semaphores, uint32_t image_idx) const;
private:
	VkSwapchainKHR swapchain;
	std::string name;
	vkb::Swapchain vkb_swapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	const VKW_Queue* present_queue;

	std::vector<VkImage> get_images();
	std::vector<VkImageView> get_image_views();
public:
	inline VkSwapchainKHR get_swapchain() const { return swapchain; };
	inline operator VkSwapchainKHR() const { return swapchain; };

	inline size_t size() const { return vkb_swapchain.image_count; };
	inline const VkImage& images_at(size_t i) const { return images.at(i); };
	inline const VkImageView& image_views_at(size_t i) const { return image_views.at(i); };

	inline VkExtent2D get_extent() const { return vkb_swapchain.extent; };
	inline const VkFormat& get_format() const { return vkb_swapchain.image_format; };
};

