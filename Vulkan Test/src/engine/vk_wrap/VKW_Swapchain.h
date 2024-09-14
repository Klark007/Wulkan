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
public:
  inline VkSwapchainKHR get_swapchain() const { return swapchain.swapchain; };
  inline VkExtent2D get_extent() const { return swapchain.extent; };
  inline VkFormat get_format() const { return swapchain.image_format; };
};

