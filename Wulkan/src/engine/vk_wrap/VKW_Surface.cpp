#include "common.h"
#include "VKW_Surface.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

void VKW_Surface::init(GLFWwindow* window, const VKW_Instance* vkw_instance)
{
    instance = vkw_instance;
    VK_CHECK_ET(glfwCreateWindowSurface(*instance, window, nullptr, &surface), SetupException, "Failed to create window surface");
}

void VKW_Surface::del()
{
    VK_DESTROY(surface, vkDestroySurfaceKHR, *instance, surface);
}
