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
        e.init(WIDTH, HEIGHT);
        engine = &e;
        window = engine->get_window();

        initVulkan();
        initImGui();

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

    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkCommandPool transferCommandPool;
    

    VKW_GraphicsPipeline graphics_pipeline;

    // done
    std::vector<VKW_Buffer> uniform_buffers;
    std::vector<void*> uniformBuffersMapped;
    UniformBufferObject ubo{};

    VKW_DescriptorPool descriptor_pool;
    std::vector<VKW_DescriptorSet> descriptor_sets;
    VKW_DescriptorSetLayout descriptor_set_layout;

    VKW_PushConstants<PushConstData> push_constants;

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

        //initModel();
        
        createSwapChain();
        
        createDescriptorSetLayout();
        
        //createDepthResources();
        
        //createTextureImage();
        //createTextureSampler();

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
        init_info.DescriptorPool = engine->imgui_descriptor_pool;

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
        vert_shader.init(&engine->device, "shaders/old/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

        VKW_Shader frag_shader{};
        //frag_shader.init(&engine->device, "shaders/triangle_f.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        frag_shader.init(&engine->device, "shaders/old/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VKW_Shader tess_ctrl_shader{};
        tess_ctrl_shader.init(&engine->device, "shaders/old/terrain_tcs.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

        VKW_Shader tess_eval_shader{};
        tess_eval_shader.init(&engine->device, "shaders/old/terrain_tes.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

        // new

        graphics_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
        //graphics_pipeline.set_wireframe_mode();
        graphics_pipeline.set_culling_mode(VK_CULL_MODE_NONE);
        graphics_pipeline.enable_depth_test();
        graphics_pipeline.enable_depth_write();

        graphics_pipeline.enable_tesselation();
        graphics_pipeline.set_tesselation_patch_size(4);

        graphics_pipeline.add_shader_stages({vert_shader, frag_shader, tess_ctrl_shader, tess_eval_shader});
        //graphics_pipeline.add_shader_stages({vert_shader, frag_shader});
        graphics_pipeline.add_descriptor_sets({descriptor_set_layout});
        graphics_pipeline.add_push_constant(push_constants);

        graphics_pipeline.set_color_attachment_format(engine->color_render_target.get_format());
        graphics_pipeline.set_depth_attachment_format(engine->depth_render_target.get_format());

        graphics_pipeline.init(&engine->device);

        // end new

        graphics_pipeline.set_color_attachment(
            engine->color_render_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
            true,
            { {0.0f, 0.0f, 0.0f, 1.0f} }
        );

        graphics_pipeline.set_depth_attachment(
            engine->depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
            true,
            1.0f
        );


        vert_shader.del();
        frag_shader.del();
        tess_ctrl_shader.del();
        tess_eval_shader.del();
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

            uniform_buffers.at(i).map();
            uniformBuffersMapped.at(i) = uniform_buffers.at(i).get_mapped_address();
        }
    }

    void createDescriptorPool() {
        descriptor_pool.add_layout(descriptor_set_layout, MAX_FRAMES_IN_FLIGHT);
        descriptor_pool.init(&engine->device, MAX_FRAMES_IN_FLIGHT);
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
            descriptor_sets[i].update(1, engine->terrain.height_map, engine->linear_texture_sampler, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        push_constants.init(VK_SHADER_STAGE_VERTEX_BIT);
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

        updateUniformBuffer(engine->current_frame);

        recordCommand(engine->command_structs.at(engine->current_frame).graphics_command_buffer, imageIndex, drawImGui());


        // submit command buffer
        engine->get_current_command_buffer().submit(
            { engine->get_current_swapchain_semaphore() },
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { engine->get_current_render_semaphore() },
            engine->get_current_render_fence()
        );
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
        //color_texture.del();
        //depth_texture.del();
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
        //createDepthResources();
    }

    void recordCommand(const VKW_CommandBuffer& commandBuffer, uint32_t imageIndex, ImDrawData* imgui_data) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        Texture::transition_layout(commandBuffer, engine->color_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        Texture::transition_layout(commandBuffer, engine->depth_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        /*
        graphics_pipeline.set_render_size(swapChainExtent);
        
        graphics_pipeline.begin_rendering(commandBuffer);
        graphics_pipeline.bind(commandBuffer);
            graphics_pipeline.set_dynamic_viewport(commandBuffer);
            graphics_pipeline.set_dynamic_scissor(commandBuffer);

            descriptor_sets[engine->current_frame].bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get_layout());
            push_constants.update({ engine->terrain.mesh.get_vertex_address() });
            push_constants.push(commandBuffer, graphics_pipeline.get_layout());

            engine->terrain.mesh.draw(commandBuffer);
            
        graphics_pipeline.end_rendering(commandBuffer);
        */
        engine->terrain_pipeline.set_render_size(swapChainExtent);

        engine->terrain_pipeline.begin_rendering(commandBuffer);
        engine->terrain_pipeline.bind(commandBuffer);
        

        engine->terrain_pipeline.set_dynamic_viewport(commandBuffer);
        engine->terrain_pipeline.set_dynamic_scissor(commandBuffer);
        engine->terrain.draw(commandBuffer, engine->current_frame, engine->terrain_pipeline);
        
        engine->terrain_pipeline.end_rendering(commandBuffer);
        
 

        Texture::transition_layout(commandBuffer, engine->color_render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkExtent2D extend{ swapChainExtent.width, swapChainExtent.height };
        Texture::copy(commandBuffer, engine->color_render_target, engine->swapchain.images_at(imageIndex), extend, extend);

        // imgui draws directly into the swapchain image (don't care about post it in post processing)
        Texture::transition_layout(commandBuffer, engine->swapchain.images_at(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);


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

    void updateUniformBuffer(uint32_t currentFrame) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        ubo.model = glm::scale(glm::mat4(1), glm::vec3(25.0,25.0,1.0)); // important not to scale y for error calculations
        ubo.view  = engine->camera.generate_view_mat(); 
        ubo.virtualView  = engine->camera.generate_virtual_view_mat();

        ubo.proj  = engine->camera.generate_projection_mat();
        // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication
        ubo.proj[1][1] *= -1;

        ubo.tesselationStrength = 24; //abs(cos(time/10)) * 64; // max innerTess is found using limits.maxTessellationGenerationLevel
        ubo.heightScale = 0.6f * 25;

        memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    void cleanup() {
        cleanupSwapChain();
        cleanupImGui();
        
        //height_map.del();

        descriptor_pool.del();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uniform_buffers[i].unmap();
            uniform_buffers[i].del();
        }

        
        descriptor_set_layout.del();

        //mesh.del();

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