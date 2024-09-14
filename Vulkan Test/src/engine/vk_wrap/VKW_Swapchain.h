#pragma once

#include "../vk_types.h"

#include "VKW_Device.h"

class VKW_Swapchain
{
public:
  VKW_Swapchain(struct GLFWwindow* window, std::shared_ptr<VKW_Device> device);
  ~VKW_Swapchain();
  // TODO: add resizing with using the old swapchain
private:
  vkb::Swapchain swapchain;
  std::vector<VkImageView> image_views;

  std::vector<VkImage> get_images();
  std::vector<VkImageView> get_image_views();
public:
  inline VkSwapchainKHR get_swapchain() const { return swapchain.swapchain; };
  inline size_t size() const { return swapchain.image_count; };
  inline const VkImageView& at(size_t i) const { return image_views.at(i); };

  inline VkExtent2D get_extent() const { return swapchain.extent; };
  inline VkFormat get_format() const { return swapchain.image_format; };
};

