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

std::shared_ptr<Camera> camera = std::make_shared<Camera>(glm::vec3(0.0, 12.0, 8.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), WIDTH, HEIGHT, glm::radians(45.0f), 0.01f, 100.0f);

Engine e{ WIDTH, HEIGHT, camera };
Engine* engine = nullptr;

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y);

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %s\n", string_VkResult(err));
    if (err < 0)
        abort();
}

struct PushConstData {
    VkDeviceAddress vertex_buffer;
};

// TODO: use #define to enable / disable virtualView, add specialization constant to choose view matrix used
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 virtualView;
    alignas(16) glm::mat4 proj;
    alignas(4) float tesselationStrength;
    alignas(4) float heightScale;
};

class HelloTriangleApplication {
public:
    void run() {
        engine = &e;
        window = engine->get_window();

        initVulkan();
        initImGui();

        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

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

    Texture color_texture;
    Texture depth_texture;

    VKW_GraphicsPipeline graphics_pipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkCommandPool transferCommandPool;
    
    std::vector<VKW_Buffer> uniform_buffers;
    std::vector<void*> uniformBuffersMapped;

    VKW_DescriptorPool descriptor_pool;
    std::vector<VKW_DescriptorSet> descriptor_sets;
    VKW_DescriptorSetLayout descriptor_set_layout;

    VKW_PushConstants<PushConstData> push_constants;

    Texture height_map;
    VKW_Sampler texture_sampler;

    Mesh mesh;

    UniformBufferObject ubo{};

    void initModel() {
        std::vector<Vertex> model_vertices{};

        //model_vertices.push_back({ { 1,1,0 }, 0.0f, { 0,1,0 }, 0.0f, { 1,0,1,1 } });
        //model_vertices.push_back({ { -1,1,0 }, 0.0f, { 0,1,0 }, 0.0f, { 1,0,1,1 } });
        //model_vertices.push_back({ { 0,-1,0 }, 0.0f, { 0,1,0 }, 0.0f, { 0,1,0,1 } });

        
        const uint32_t mesh_res = 32;
        
        for (uint32_t iy = 0; iy < mesh_res; iy++) {
            for (uint32_t ix = 0; ix < mesh_res; ix++) {
                float u = ((float)ix) / mesh_res;
                float v = ((float)iy) / mesh_res;
                float pos_x = (u - 0.5) * 2;
                float pos_z = (v - 0.5) * 2;

                model_vertices.push_back({ {pos_x,0,pos_z}, u, {0,1,0}, v, {1,0,0,1} });
            }
        }

        std::vector<uint32_t> model_indices{};
        //model_indices.push_back(0);
        //model_indices.push_back(1);
        //model_indices.push_back(2);
        
        for (uint32_t iy = 0; iy < mesh_res-1; iy++) {
            for (uint32_t ix = 0; ix < mesh_res-1; ix++) {
                model_indices.push_back(ix + iy * mesh_res);
                model_indices.push_back(ix+1 + iy * mesh_res);
                model_indices.push_back(ix+1 + (iy+1) * mesh_res);
                model_indices.push_back(ix + (iy+1) * mesh_res);
            }
        }

        mesh.init(engine->device, engine->get_current_transfer_pool(), model_vertices, model_indices);
        
    }

    void initVulkan() {
        instance = engine->instance;

        surface = engine->surface;

        physicalDevice = engine->device.get_physical_device();
        device = engine->device;

        graphicsQueueFamily = engine->graphics_queue.get_queue_family();
        std::cout << "Chosen queues" << engine->graphics_queue.get_queue_family() << "," << engine->present_queue.get_queue_family() << "," << engine->transfer_queue.get_queue_family() << std::endl;
        graphicsQueue = engine->graphics_queue;
        presentQueue = engine->present_queue;
        transferQueue = engine->transfer_queue;

        initModel();
        
        createSwapChain();
        
        createDescriptorSetLayout();
        
        createDepthResources();
        
        createTextureImage();
        createTextureSampler();

        createUniformBuffers();
        
        createDescriptorPool();
        createDescriptorSets();

        createGraphicsPipeline();
    }

    void initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = graphicsQueueFamily;
        init_info.Queue = graphicsQueue;
        init_info.DescriptorPool = descriptor_pool;

        //dynamic rendering parameters for imgui to use
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;

        init_info.MinImageCount = engine->swapchain.size();
        init_info.ImageCount = engine->swapchain.size();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);
    }

    void createSwapChain() {
        swapChain = engine->swapchain;
        swapChainImageFormat = engine->swapchain.get_format();
        swapChainExtent = engine->swapchain.get_extent();
    }

    void createDescriptorSetLayout() {
        descriptor_set_layout.add_binding(
            0, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
        );

        descriptor_set_layout.add_binding(
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
        );

        descriptor_set_layout.init(&engine->device);
    }

    void createGraphicsPipeline() {
        VKW_Shader vert_shader{};
        //vert_shader.init(&engine->device, "shaders/triangle_v.spv", VK_SHADER_STAGE_VERTEX_BIT);
        vert_shader.init(&engine->device, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

        VKW_Shader frag_shader{};
        //frag_shader.init(&engine->device, "shaders/triangle_f.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        frag_shader.init(&engine->device, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VKW_Shader tess_ctrl_shader{};
        tess_ctrl_shader.init(&engine->device, "shaders/terrain_tcs.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

        VKW_Shader tess_eval_shader{};
        tess_eval_shader.init(&engine->device, "shaders/terrain_tes.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

        // new

        graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
        graphics_pipeline.set_wireframe_mode();
        graphics_pipeline.set_culling_mode(VK_CULL_MODE_NONE);
        graphics_pipeline.enable_depth_test();
        graphics_pipeline.enable_depth_write();

        graphics_pipeline.enable_tesselation();
        graphics_pipeline.set_tesselation_patch_size(4);

        graphics_pipeline.add_shader_stages({vert_shader, frag_shader, tess_ctrl_shader, tess_eval_shader});
        //graphics_pipeline.add_shader_stages({vert_shader, frag_shader});
        graphics_pipeline.add_descriptor_sets({descriptor_set_layout});
        graphics_pipeline.add_push_constant(push_constants);

        graphics_pipeline.set_color_attachment_format(color_texture.get_format());
        graphics_pipeline.set_depth_attachment_format(depth_texture.get_format());

        graphics_pipeline.init(&engine->device);

        // end new


        vert_shader.del();
        frag_shader.del();
        tess_ctrl_shader.del();
        tess_eval_shader.del();
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        depth_texture.init(
            &engine->device,
            swapChainExtent.width, swapChainExtent.height,
            depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            sharing_exlusive()
        );

        color_texture.init(
            &engine->device,
            swapChainExtent.width, swapChainExtent.height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            sharing_exlusive()
        );
    }

    void createTextureImage() {
        height_map = create_texture_from_path(
            &engine->device,
            &engine->get_current_graphics_pool(),
            "textures/perlinNoise.png",
            Texture_Type::Tex_R
        );
    }

    void createTextureSampler() {
        texture_sampler.init(&engine->device);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uniform_buffers.at(i).init(
                &engine->device,
                bufferSize, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                sharing_exlusive(),
                true
            );

            uniformBuffersMapped.at(i) = uniform_buffers.at(i).map();
        }
    }

    void createDescriptorPool() {
        descriptor_pool.add_layout(descriptor_set_layout, MAX_FRAMES_IN_FLIGHT);
        // 1 additional sampler for imgui as well
        descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
        descriptor_pool.init(&engine->device, MAX_FRAMES_IN_FLIGHT + 1);
    }

    void createDescriptorSets() {
        // allocate descriptors
        for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VKW_DescriptorSet set{};
            set.init(&engine->device, &descriptor_pool, descriptor_set_layout);
            descriptor_sets.push_back(set);
        }

        // configure descriptors
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            descriptor_sets[i].update(0, uniform_buffers[i]);
            descriptor_sets[i].update(1, height_map, texture_sampler, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        push_constants.init(VK_SHADER_STAGE_VERTEX_BIT);
    }

    void mainLoop() {

        while (!glfwWindowShouldClose(window)) {
            engine->update();
            engine->draw();
            drawFrame();
            engine->late_update();
        }

        vkDeviceWaitIdle(device);
    }

    void drawFrame() {
        /*
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error(std::string(string_VkResult(result))+":Failed to aquire swap chain image");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]); // only reset once work is submitted to avoid deadlock
        */
        uint32_t imageIndex = engine->current_swapchain_image_idx; 

        updateUniformBuffer(engine->current_frame);

        vkResetCommandBuffer(engine->command_structs.at(engine->current_frame).graphics_command_buffer, 0);

        recordCommand(engine->command_structs.at(engine->current_frame).graphics_command_buffer, imageIndex, drawImGui());


        // submit command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { engine->get_current_swapchain_semaphore()};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // wait with writing colors until image available
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        VkCommandBuffer command_buffer = engine->command_structs.at(engine->current_frame).graphics_command_buffer;
        submitInfo.pCommandBuffers = &command_buffer;

        VkSemaphore signalSemaphores[] = { engine->get_current_render_semaphore() };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, engine->get_current_render_fence()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw commandbuffer");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->resize_window) {
            engine->resize_window = false;
            recreateSwapChain();
        }
        else if (result) {
            throw std::runtime_error(std::string(string_VkResult(result)) + ":Failed to present swap chain image");
        }
    }

    ImDrawData* drawImGui() {
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();


        ImGui::Render();
        return ImGui::GetDrawData();
    }

    void cleanupSwapChain() {
        for (auto& framebuffer : swapChainFramebuffers) {
            if (framebuffer) {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
            }
        }
        
        color_texture.del();
        depth_texture.del();
    }

    void cleanupImGui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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
        createSwapChain();
        createDepthResources();
    }

    void recordCommand(const VKW_CommandBuffer& commandBuffer, uint32_t imageIndex, ImDrawData* imgui_data) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        Texture::transition_layout(commandBuffer, color_texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        Texture::transition_layout(commandBuffer, depth_texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


        VkRenderingAttachmentInfo color_attachment_info{};
        color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        color_attachment_info.imageView = color_texture.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT);
        color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkRenderingAttachmentInfo depth_attachment_info{};
        depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment_info.clearValue.depthStencil.depth = 1.0f;
        depth_attachment_info.imageView = depth_texture.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT);
        depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        VkRenderingInfo render_info{};
        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

        render_info.renderArea.offset = { 0, 0 };
        render_info.renderArea.extent = swapChainExtent;
        render_info.layerCount = 1;

        render_info.pColorAttachments = &color_attachment_info;
        render_info.colorAttachmentCount = 1;

        render_info.pDepthAttachment = &depth_attachment_info;

        vkCmdBeginRendering(commandBuffer, &render_info);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            descriptor_sets[engine->current_frame].bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get_layout());
            push_constants.update({ mesh.get_vertex_address() });
            push_constants.push(commandBuffer, graphics_pipeline.get_layout());

            mesh.draw(commandBuffer);
            
        vkCmdEndRendering(commandBuffer);

 

        Texture::transition_layout(commandBuffer, color_texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkExtent2D extend{ swapChainExtent.width, swapChainExtent.height };
        copy_image_to_image(commandBuffer, color_texture, engine->swapchain.images_at(imageIndex), extend, extend);


        // imgui draws directly into the swapchain image (don't care about post it in post processing)
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // might have different resolution than aboce
        VkRenderingAttachmentInfo color_attachment_info_imgui{};
        color_attachment_info_imgui.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_info_imgui.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_info_imgui.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info_imgui.imageView = engine->swapchain.image_views_at(imageIndex);
        color_attachment_info_imgui.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        render_info.renderArea.offset = { 0, 0 };
        render_info.renderArea.extent = swapChainExtent;
        render_info.pColorAttachments = &color_attachment_info_imgui;
        render_info.pDepthAttachment = VK_NULL_HANDLE;

        vkCmdBeginRendering(commandBuffer, &render_info);

        ImGui_ImplVulkan_RenderDrawData(imgui_data, commandBuffer);

        vkCmdEndRendering(commandBuffer);


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

    // todo: move to Texture...
    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
    {
        VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

        blitRegion.srcOffsets[1].x = srcSize.width;
        blitRegion.srcOffsets[1].y = srcSize.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstSize.width;
        blitRegion.dstOffsets[1].y = dstSize.height;
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcSubresource.mipLevel = 0;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstSubresource.mipLevel = 0;

        VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
        blitInfo.dstImage = destination;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage = source;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter = VK_FILTER_LINEAR;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;

        vkCmdBlitImage2(cmd, &blitInfo);
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void updateUniformBuffer(uint32_t currentFrame) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        ubo.model = glm::scale(glm::mat4(1), glm::vec3(25.0,1,25.0)); // important not to scale y for error calculations
        ubo.view  = camera->generate_view_mat(); 
        ubo.virtualView  = camera->generate_virtual_view_mat(); 

        ubo.proj  = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);

        ubo.proj[1][1] *= -1;

        ubo.tesselationStrength = 24; //abs(cos(time/10)) * 64; // max innerTess is found using limits.maxTessellationGenerationLevel
        ubo.heightScale = 0.6f * 25;

        memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    void cleanup() {
        cleanupSwapChain();
        cleanupImGui();
        
        texture_sampler.del();
        height_map.del();

        descriptor_pool.del();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uniform_buffers[i].unmap();
            uniform_buffers[i].del();
        }

        
        descriptor_set_layout.del();

        mesh.del();

        graphics_pipeline.del();
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