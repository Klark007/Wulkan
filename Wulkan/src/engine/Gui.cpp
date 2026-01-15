#include "common.h"
#include "Gui.h"

#include "DirectionalLight.h"

#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "glm/gtc/type_ptr.hpp"

#include "spdlog/spdlog.h"

static void check_imgui_result(VkResult result) {
	VK_CHECK_ET(result, RuntimeException, "IMGUI Vulkan error");
}

void GUI::init(GLFWwindow* window, const VKW_Instance& instance, const VKW_Device& device, const VKW_Queue& graphics_queue, const VKW_DescriptorPool& descriptor_pool, const VKW_Swapchain* vkw_swapchain, CameraController* camera_controller, std::recursive_mutex* camera_recursive_mutex)
{
	m_camera_controller = camera_controller;
	m_camera_recursive_mutex = camera_recursive_mutex;

	m_swapchain = vkw_swapchain;
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
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain->get_format();

	init_info.MinImageCount = static_cast<uint32_t>(m_swapchain->size());
	init_info.ImageCount = static_cast<uint32_t>(m_swapchain->size());
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
	color_attachment_info_imgui.imageView = m_swapchain->image_views_at(image_idx);
	color_attachment_info_imgui.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkRenderingInfo imgui_render_info{};
	imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	imgui_render_info.renderArea.offset = { 0, 0 };
	imgui_render_info.renderArea.extent = m_swapchain->get_extent();
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
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::SliderFloat("Movement speed", &m_data.camera_movement_speed, 5.0f, 50.0f);
			ImGui::SliderFloat("Rotation speed", &m_data.camera_rotation_speed, 0.0005f, 0.005f);
			ImGui::SliderFloat("Near plane", &m_data.camera_near_plane, 0.01f, 0.2f);
			ImGui::SliderFloat("Far plane", &m_data.camera_far_plane, 10.0f, 250.0f);

			// TODO: support smt like https://github.com/btzy/nativefiledialog-extended
			m_camera_recursive_mutex->lock();
			static char path[256] = "out/pose.csv";
			ImGui::InputText("Camera pose path:", path, IM_ARRAYSIZE(path));
			if (ImGui::Button("Store position")) {
				spdlog::info("Storing camera pose into {}", path);
				m_camera_controller->export_active_camera(path);
			}

			if (ImGui::Button("Load position")) {
				spdlog::info("Storing camera pose into {}", path);
				m_camera_controller->import_active_camera(path);
			}
			m_camera_recursive_mutex->unlock();
		}

		if (ImGui::CollapsingHeader("Terrain")) {
			ImGui::SliderFloat("Tesselation scale", &m_data.terrain_tesselation, 1.0f, 50.0f);
			ImGui::SliderFloat("Maximum Tesselation ammount", &m_data.max_terrain_tesselation, 1.0f, 100.0f);
			ImGui::SliderFloat("Height Scale", &m_data.terrain_height_scale, 0.1f, 1.0f);
			ImGui::SliderFloat("Texture Derivative epsilon", &m_data.terrain_texture_eps, 0.01f, 1.0f);

			ImGui::Checkbox("Show Wireframe", &m_data.terrain_wireframe_mode);

			constexpr const char* visualization_modes[] = { "Shaded", "Level", "Height", "Normals", "Error", "Shadow map cascade"};
			static int selected_vis = 0;
			if (ImGui::TreeNode("Visualization mode")) {
				ImGui::ListBox("Mode", &selected_vis, visualization_modes, IM_ARRAYSIZE(visualization_modes));
				m_data.terrain_vis_mode = static_cast<TerrainVisualizationMode>(selected_vis);
					
				ImGui::TreePop();
			}
		}

		if (ImGui::CollapsingHeader("Shading")) {
			ImGui::SliderFloat3("Sun direction", glm::value_ptr(m_data.sun_direction), -1.0f, 1.0f);

			ImGui::ColorEdit3("Sun Color", glm::value_ptr(m_data.sun_color), ImGuiColorEditFlags_None);
			ImGui::SliderFloat("Sun Intensity", &m_data.sun_intensity, 0.1f, 25.0f);

			ImGui::Checkbox("Draw Trees", &m_data.draw_trees);

			if (ImGui::TreeNode("Tone mapper")) {
				constexpr const char* tone_mapper_modes[] = { "None", "Rheinhard", "Extended Rheinhard", "Uncharted", "ACES", "AgX"};
				static int selected_tone_mapper = static_cast<int>(m_data.tone_mapper_mode);
				ImGui::ListBox("Tone Mapper", &selected_tone_mapper, tone_mapper_modes, IM_ARRAYSIZE(tone_mapper_modes));
				m_data.tone_mapper_mode = static_cast<ToneMapperMode>(selected_tone_mapper);

				ImGui::DragFloat("Luminance White Point", &m_data.luminance_white_point, 0.01f, 0, FLT_MAX);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Visualization modes")) {
				constexpr const char* visualization_modes[] = { "Shaded", "Normals", "Diffuse", "Shadow map cascade", "LOD Level" };
				static int selected_vis = 0;
				ImGui::ListBox("PBR Mode", &selected_vis, visualization_modes, IM_ARRAYSIZE(visualization_modes));
				m_data.pbr_vis_mode = static_cast<VisualizationMode>(selected_vis);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Shadows")) {
				constexpr const char* shadow_modes[] = { "No shadows", "Hard shadows", "Soft Shadows" };
				static int selected_shadow_mode = 2;
				ImGui::ListBox("Mode", &selected_shadow_mode, shadow_modes, IM_ARRAYSIZE(shadow_modes));
				m_data.shadow_mode = static_cast<ShadowMode>(selected_shadow_mode);

				ImGui::SliderInt("Number of shadow cascades", &m_data.nr_shadow_cascades, 1, MAX_CASCADE_COUNT);
				ImGui::SliderFloat("Constant depth bias", &m_data.depth_bias, 1e4, 1e5);
				ImGui::SliderFloat("Slope depth bias", &m_data.slope_depth_bias, 1e-2f, 1e1);

				ImGui::SliderInt("Nr Receiver Samples", &m_data.nr_shadow_receiver_samples, 1, 32);
				ImGui::SliderInt("Nr Occluder Samples", &m_data.nr_shadow_occluder_samples, 1, 16);
				ImGui::SliderFloat("Receiver Occlusion Sample Region", &m_data.receiver_sample_region, 1.0f, 64.0f);
				ImGui::SliderFloat("Occluder Depth Sample Region", &m_data.occluder_sample_region, 1.0f, 64.0f);

				ImGui::Checkbox("Show debug frustums", &m_data.shadow_draw_debug_frustums);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("LOD")) {
				ImGui::SliderFloat("Distance ratio LOD 0", &m_data.lod_ratios[0], 0, m_data.lod_ratios[1]);
				ImGui::SliderFloat("Distance ratio LOD 1", &m_data.lod_ratios[1], m_data.lod_ratios[0], m_data.lod_ratios[2]);
				ImGui::SliderFloat("Distance ratio LOD 2", &m_data.lod_ratios[2], m_data.lod_ratios[1], 1);
				ImGui::TreePop();
			}
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
	ImGui::End();


	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
