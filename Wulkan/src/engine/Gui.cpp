#include "Gui.h"

static void check_imgui_result(VkResult result) {
	if (result) {
		std::cout << std::string(string_VkResult(result)) + ";\n" + " IMGUI Vulkan error" << std::endl;
		abort();
	}
}

void GUI::init(GLFWwindow* window, const VKW_Instance& instance, const VKW_Device& device, const VKW_Queue& graphics_queue, const VKW_DescriptorPool& descriptor_pool, const VKW_Swapchain* vkw_swapchain)
{
	swapchain = vkw_swapchain;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = device.get_physical_device();
	init_info.Device = device;
	init_info.QueueFamily = graphics_queue.get_queue_family();
	init_info.Queue = graphics_queue;
	init_info.DescriptorPool = descriptor_pool;

	// toggle dynamic rendering
	init_info.UseDynamicRendering = true;
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain->get_format();

	init_info.MinImageCount = swapchain->size();
	init_info.ImageCount = swapchain->size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.CheckVkResultFn = check_imgui_result;
	ImGui_ImplVulkan_Init(&init_info);
}

void GUI::del()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GUI::draw(const VKW_CommandBuffer& cmd, uint32_t image_idx)
{
	VkRenderingAttachmentInfo color_attachment_info_imgui{};
	color_attachment_info_imgui.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment_info_imgui.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	color_attachment_info_imgui.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info_imgui.imageView = swapchain->image_views_at(image_idx);
	color_attachment_info_imgui.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkRenderingInfo imgui_render_info{};
	imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	imgui_render_info.renderArea.offset = { 0, 0 };
	imgui_render_info.renderArea.extent = swapchain->get_extent();
	imgui_render_info.layerCount = 1;

	imgui_render_info.pColorAttachments = &color_attachment_info_imgui;
	imgui_render_info.colorAttachmentCount = 1;
	imgui_render_info.pDepthAttachment = VK_NULL_HANDLE;


	vkCmdBeginRendering(cmd, &imgui_render_info);

	draw_gui(cmd);

	vkCmdEndRendering(cmd);
}

void GUI::draw_gui(const VKW_CommandBuffer& cmd)
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Wulkan GUI");  // creates window

	{
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::TreeNode("Camera movement")) {
				ImGui::SliderFloat("Movement speed", &data.camera_movement_speed, 1.0f, 25.0f);
				ImGui::SliderFloat("Rotation speed", &data.camera_rotation_speed, 0.0005f, 0.005f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Terrain")) {
				ImGui::SliderFloat("Tesselation scale", &data.terrain_tesselation, 1.0f, 50.0f);
				ImGui::SliderFloat("Maximum Tesselation ammount", &data.max_terrain_tesselation, 1.0f, 100.0f);
				ImGui::SliderFloat("Height Scale", &data.terrain_height_scale, 0.1f, 1.0f);
				ImGui::SliderFloat("Texture Derivative epsilon", &data.terrain_texture_eps, 0.01f, 1.0f);

				constexpr const char* visualization_modes[] = { "Shaded", "Level", "Height", "Normals", "Error"};
				static int selected_vis = 0;
				if (ImGui::TreeNode("Visualization mode")) {
					ImGui::ListBox("Mode", &selected_vis, visualization_modes, IM_ARRAYSIZE(visualization_modes));
					data.terrain_vis_mode = static_cast<TerrainVisualizationMode>(selected_vis);
					
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
	ImGui::End();


	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
