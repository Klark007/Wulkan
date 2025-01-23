#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "engine/vk_types.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/Engine.h"
#include "engine/Camera.h"

#include <chrono>

#include <iostream>
#include <stdexcept>

#include <cstdlib>
#include <cstdint>
#include <cmath>

#include <limits>
#include <algorithm>
#include <fstream>


// const unsigned int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

//std::shared_ptr<Camera> camera = std::make_shared<Camera>(glm::vec3(0.0, 12.0, 8.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), WIDTH, HEIGHT, glm::radians(45.0f), 0.01f, 100.0f);

Engine e;
Engine* engine = nullptr;

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);

class HelloTriangleApplication {
public:
    void run() {
        e.init(WIDTH, HEIGHT);
        engine = &e;
        window = engine->get_window();

        initVulkan();

        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkDevice device;

    uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    
    void initVulkan() {
        instance = engine->instance;

        surface = engine->surface;

        physicalDevice = engine->device.get_physical_device();
        device = engine->device;

        graphicsQueueFamily = engine->graphics_queue.get_queue_family();
        //std::cout << "Chosen queues" << engine->graphics_queue.get_queue_family() << "," << engine->present_queue.get_queue_family() << "," << engine->transfer_queue.get_queue_family() << std::endl;
        graphicsQueue = engine->graphics_queue;
        presentQueue = engine->present_queue;
        transferQueue = engine->transfer_queue;

        swapChain = engine->swapchain;
        swapChainImageFormat = engine->swapchain.get_format();
        swapChainExtent = engine->swapchain.get_extent();
    }

    void mainLoop() {

        while (!glfwWindowShouldClose(window)) {
            engine->update();
            engine->draw();
            drawFrame();
            engine->present();
            engine->late_update();
        }

        vkDeviceWaitIdle(device);
    }

    void drawFrame() {
        uint32_t imageIndex = engine->current_swapchain_image_idx; 

        recordCommand(engine->command_structs.at(engine->current_frame).graphics_command_buffer, imageIndex);


        // submit command buffer
        engine->get_current_command_buffer().submit(
            { engine->get_current_swapchain_semaphore() },
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { engine->get_current_render_semaphore() },
            engine->get_current_render_fence()
        );
    }

    void cleanupSwapChain() {        
        //color_texture.del();
        //depth_texture.del();
    }

    void recreateSwapChain() {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        engine->create_swapchain();
        //createDepthResources();
    }

    void recordCommand(const VKW_CommandBuffer& commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        Texture::transition_layout(commandBuffer, engine->color_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        Texture::transition_layout(commandBuffer, engine->depth_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);


        engine->terrain_pipeline.set_render_size(swapChainExtent);

        engine->terrain_pipeline.begin_rendering(commandBuffer);
        {
            engine->terrain_pipeline.bind(commandBuffer);
        

            engine->terrain_pipeline.set_dynamic_viewport(commandBuffer);
            engine->terrain_pipeline.set_dynamic_scissor(commandBuffer);
            engine->terrain.draw(commandBuffer, engine->current_frame, engine->terrain_pipeline);
        }
        engine->terrain_pipeline.end_rendering(commandBuffer);
        
 

        Texture::transition_layout(commandBuffer, engine->color_render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        Texture::copy(commandBuffer, engine->color_render_target, engine->swapchain.images_at(imageIndex), swapChainExtent, swapChainExtent);

        // imgui draws directly into the swapchain image (don't care about post it in post processing)
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        engine->gui.draw(commandBuffer, imageIndex);
        /*
        // might have different resolution than aboce
        VkRenderingAttachmentInfo color_attachment_info_imgui{};
        color_attachment_info_imgui.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_info_imgui.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_info_imgui.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info_imgui.imageView = engine->swapchain.image_views_at(imageIndex);
        color_attachment_info_imgui.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkRenderingInfo imgui_render_info{};
        imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        imgui_render_info.renderArea.offset = { 0, 0 };
        imgui_render_info.renderArea.extent = swapChainExtent;
        imgui_render_info.layerCount = 1;
        imgui_render_info.pColorAttachments = &color_attachment_info_imgui;
        imgui_render_info.colorAttachmentCount = 1;
        imgui_render_info.pDepthAttachment = VK_NULL_HANDLE;

        vkCmdBeginRendering(commandBuffer, &imgui_render_info);

        ImGui_ImplVulkan_RenderDrawData(imgui_data, commandBuffer);

        vkCmdEndRendering(commandBuffer);
        */


        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("Supported format not found");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void cleanup() {
        cleanupSwapChain();
    }
};

int main() {
    HelloTriangleApplication app {};

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        std::cout << "Press anything to close:";
        char c;
        std::cin >> c;
        
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}