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
void VKW_Swapchain::init(GLFWwindow* window, const VKW_Device& device, const VKW_Queue* queue, const std::string& obj_name, VkSwapchainKHR old_swapchain)
{
    name = obj_name;
    present_queue = queue;

    vkb::SwapchainBuilder builder{ device.get_vkb_device() };
  
    // actual pixel resolution, might differ from given size which is screen coordinates
    // see https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
  
    builder.set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .set_old_swapchain(old_swapchain)
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // V Sync
        .set_required_min_image_count(MAX_FRAMES_IN_FLIGHT) // This might cause issue if a device only supports at least tripple buffered images
    ;

    auto build_result = builder.build();

    if (!build_result) {
        throw SetupException(std::string(string_VkResult((VkResult) build_result.vk_result())) + ";\n" + "Failed to create a swapchain", __FILE__, __LINE__);
    }

    vkb_swapchain = build_result.value();
    swapchain = vkb_swapchain.swapchain;
    images = get_images();
    image_views = get_image_views();

    device.name_object((uint64_t)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
}

void VKW_Swapchain::del()
{
    if (swapchain) {
        vkb_swapchain.destroy_image_views(image_views);
        vkb::destroy_swapchain(vkb_swapchain);

        swapchain = VK_NULL_HANDLE;
    }
}

void VKW_Swapchain::recreate(GLFWwindow* window, const VKW_Device& device)
{
    vkb_swapchain.destroy_image_views(image_views);
    vkb::Swapchain old_swapchain = vkb_swapchain;
    
    init(window, device, present_queue, name, old_swapchain);

    vkb::destroy_swapchain(old_swapchain);
}


bool VKW_Swapchain::present(const std::vector<VkSemaphore>& wait_semaphores, uint32_t image_idx) const
{
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.pSwapchains = &swapchain;
    present_info.swapchainCount = 1;
    
    present_info.pWaitSemaphores = wait_semaphores.data();
    present_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());

    present_info.pImageIndices = &image_idx;

    VkResult result = vkQueuePresentKHR(*present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    else if (result) {
        throw RuntimeException(std::string(string_VkResult(result)) + ";\n" + "Failed to present swap chain image", __FILE__, __LINE__);
    }

    return true;
}

std::vector<VkImage> VKW_Swapchain::get_images()
{
    auto image_result = vkb_swapchain.get_images();

    if (!image_result) {
        throw SetupException("Failed to get swapchain images: " + image_result.error().message(), __FILE__, __LINE__);
    }
    return image_result.value();
}

std::vector<VkImageView> VKW_Swapchain::get_image_views()
{
    auto view_result = vkb_swapchain.get_image_views();
  
    if (!view_result) {
        throw SetupException("Failed to get swapchain image views: " + view_result.error().message(), __FILE__, __LINE__);
    }
    return view_result.value();
}
