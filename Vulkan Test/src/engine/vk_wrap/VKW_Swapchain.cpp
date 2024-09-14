#include "VKW_Swapchain.h"

#include "GLFW/glfw3.h"

/* 
 By default the swapchain builder uses
 
 * VK_FORMAT_B8G8R8A8_SRGB or VK_FORMAT_R8G8B8A8_SRGB format with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR colorspace
 * VK_PRESENT_MODE_MAILBOX_KHR or VK_PRESENT_MODE_FIFO_KHR as fall back
 * Default flag VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT

It also handles:
 * Getting correct extend
 * Respecting min/max nr of images
 * Setting preffered nr images to capabilites.minImageCount + 1
 * Sharing modes

*/
VKW_Swapchain::VKW_Swapchain(GLFWwindow* window, std::shared_ptr<VKW_Device> device)
{
  vkb::SwapchainBuilder builder{ device->get_vkb_device() };
  
  // actual pixel resolution, might differ from given size which is screen coordinates
  // see https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  
  builder.set_desired_extent(width, height)
    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
  ;

  auto build_result = builder.build();

  if (!build_result) {
    throw SetupException("Failed to create a swapchain: " + build_result.error().message(), __FILE__, __LINE__);
  }

  swapchain = build_result.value();
}

VKW_Swapchain::~VKW_Swapchain()
{
  vkb::destroy_swapchain(swapchain);
}
