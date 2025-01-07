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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value(); };
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    };
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    };
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

/*
const std::vector<Vertex> vertices = {
    {{-0.5f, 0.0, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 3
};
*/

class HelloTriangleApplication {
public:


    void run() {
        initModel();

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

    Texture depth_texture;

    VkRenderPass renderPass;
    VKW_DescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkCommandPool transferCommandPool;
    
    VKW_Buffer vertex_buffer;
    VKW_Buffer index_buffer;
    
    std::vector<VKW_Buffer> uniform_buffers;
    std::vector<void*> uniformBuffersMapped;

    VKW_DescriptorPool descriptor_pool;
    std::vector<VKW_DescriptorSet> descriptor_sets;

    Texture height_map;
    VKW_Sampler texture_sampler;

    std::vector<Vertex> model_vertices;
    std::vector<uint16_t> model_indices;

    UniformBufferObject ubo{};

    void initModel() {
        model_vertices = std::vector<Vertex>();

        const uint32_t mesh_res = 32;
        
        for (uint32_t iy = 0; iy < mesh_res; iy++) {
            for (uint32_t ix = 0; ix < mesh_res; ix++) {
                float u = ((float)ix) / mesh_res;
                float v = ((float)iy) / mesh_res;
                float pos_x = (u - 0.5) * 2;
                float pos_z = (v - 0.5) * 2;

                model_vertices.push_back({ {pos_x,0,pos_z}, {0,0,0}, {u,v} });
            }
        }

        model_indices = std::vector<uint16_t>();

        for (uint32_t iy = 0; iy < mesh_res-1; iy++) {
            for (uint32_t ix = 0; ix < mesh_res-1; ix++) {
                model_indices.push_back(ix + iy * mesh_res);
                model_indices.push_back(ix+1 + iy * mesh_res);
                model_indices.push_back(ix+1 + (iy+1) * mesh_res);
                model_indices.push_back(ix + (iy+1) * mesh_res);
            }
        }
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

        
        createSwapChain();
        
        createRenderPass();
        
        createDescriptorSetLayout();
        createGraphicsPipeline();
        
        createDepthResources();
        createFrameBuffers(); // needs depth texture
        
        createTextureImage();
        createTextureSampler();
        
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        
        createDescriptorPool();
        createDescriptorSets();
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
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;
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

    void createRenderPass() {
        VkAttachmentDescription colorAttachement{};
        colorAttachement.format = swapChainImageFormat;
        colorAttachement.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear before reading
        colorAttachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of image before renderpass
        colorAttachement.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // after renderpass

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout we want during subpass

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // not needed after subpass, might be discarded; important for post processing
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // graphics subpass
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachement, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        // dependency from previous implicit subpass to current
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }
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
        vert_shader.init(&engine->device, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

        VKW_Shader frag_shader{};
        frag_shader.init(&engine->device, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VKW_Shader tess_ctrl_shader{};
        tess_ctrl_shader.init(&engine->device, "shaders/terrain_tcs.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

        VKW_Shader tess_eval_shader{};
        tess_eval_shader.init(&engine->device, "shaders/terrain_tes.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vert_shader.get_info(), frag_shader.get_info(), tess_ctrl_shader.get_info(), tess_eval_shader.get_info()};


        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; 

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // Otherwise clamps polygon to near and farpline instead of discarding
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // Otherwise no geometry passes through frame buffer
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE; // VK_POLYGON_MODE_FILL
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; // for terrain where one can see it from belowVK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // for uniforms
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        VkDescriptorSetLayout layout = descriptor_set_layout;
        pipelineLayoutInfo.pSetLayouts = &layout;

        VK_CHECK_T(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout");

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;

        VkPipelineTessellationStateCreateInfo tesselationInfo{};
        tesselationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tesselationInfo.patchControlPoints = 4;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pTessellationState = &tesselationInfo;

        pipelineInfo.layout = pipelineLayout;

        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        VK_CHECK_T(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline), "Failed to create graphics pipeline");

        vert_shader.del();
        frag_shader.del();
        tess_ctrl_shader.del();
        tess_eval_shader.del();
    }

    void createFrameBuffers() {
        swapChainFramebuffers.resize(engine->swapchain.size());

        for (size_t i = 0; i < engine->swapchain.size(); i++) {
            std::array<VkImageView,2> attachments = {
                engine->swapchain.at(i),
                depth_texture.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT)
            };

            VkFramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = renderPass;
            frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            frameBufferInfo.pAttachments = attachments.data();
            frameBufferInfo.width  = swapChainExtent.width;
            frameBufferInfo.height = swapChainExtent.height;
            frameBufferInfo.layers = 1;
                
            if (vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
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

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(model_vertices[0]) * model_vertices.size();
        
        VKW_Buffer staging_buffer = create_staging_buffer(&engine->device, model_vertices.data(), bufferSize);

        SharingInfo sharingInfoC{
            VK_SHARING_MODE_CONCURRENT,
            {engine->graphics_queue.get_queue_family(), engine->transfer_queue.get_queue_family()}
        };
        vertex_buffer.init(
            &engine->device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            sharingInfoC,
            false
        );
        
        vertex_buffer.copy(&engine->get_current_transfer_pool(), staging_buffer);
        
        staging_buffer.del();
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(model_indices[0]) * model_indices.size();

        VKW_Buffer staging_buffer = create_staging_buffer(&engine->device, model_indices.data(), bufferSize);
        
        SharingInfo sharingInfoC{
            VK_SHARING_MODE_CONCURRENT,
            {engine->graphics_queue.get_queue_family(), engine->transfer_queue.get_queue_family()}
        };
        index_buffer.init(
            &engine->device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            sharingInfoC,
            false
        );

        index_buffer.copy(&engine->get_current_transfer_pool(), staging_buffer);

        staging_buffer.del();
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
        //createImageViews();
        createDepthResources();
        createFrameBuffers();
    }

    void recordCommand(VkCommandBuffer commandBuffer, uint32_t imageIndex, ImDrawData* imgui_data) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearColors{};
        clearColors[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearColors[1].depthStencil = {1.0f, 0};


        renderPassInfo.pClearValues = clearColors.data();
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkBuffer vertexBuffers[] = { vertex_buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

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

            descriptor_sets[engine->current_frame].bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);


            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model_indices.size()), 1, 0, 0, 0);

            ImGui_ImplVulkan_RenderDrawData(imgui_data, commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

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

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
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

        vertex_buffer.del();
        index_buffer.del();

        if (pipelineLayout) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        if (renderPass) {
            vkDestroyRenderPass(device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }

        if (graphicsPipeline) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }
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