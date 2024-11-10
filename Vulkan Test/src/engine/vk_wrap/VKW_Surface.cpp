#pragma once

#include "VKW_Surface.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

VKW_Surface::VKW_Surface(GLFWwindow* window, std::shared_ptr<VKW_Instance> instance)
    : instance { instance }
{
    VK_CHECK_ET(glfwCreateWindowSurface(*instance, window, nullptr, &surface), SetupException, "Failed to create window surface");
}

VKW_Surface::~VKW_Surface()
{
    VK_DESTROY(surface, vkDestroySurfaceKHR, *instance, surface);
}
