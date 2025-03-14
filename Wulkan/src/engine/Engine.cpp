#include "Engine.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <glm/gtc/matrix_transform.hpp>


#include <GLFW/glfw3.h>

#include <iostream>

#ifdef NDEBUG
#else
#define PROFILING
#endif

#include "Profiler.h"

void Engine::init(unsigned int w, unsigned int h)
{
	res_x = w;
	res_y = h;

	init_glfw();
	init_vulkan();

	camera = Camera( glm::vec3(0.0, 60.0, 25.0), glm::vec3(0.0, 10.0, 8.0), res_x, res_y, glm::radians(45.0f), 0.1f, 100.0f );
	camera_controller = CameraController(window, &camera);
	gui.init(window, instance, device, graphics_queue, imgui_descriptor_pool, &swapchain);
	cleanup_queue.add(&gui);
 }

Engine::~Engine()
{
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_DESTROY(sync_structs.at(i).swapchain_semaphore, vkDestroySemaphore, device, sync_structs.at(i).swapchain_semaphore);
		VK_DESTROY(sync_structs.at(i).render_semaphore, vkDestroySemaphore, device, sync_structs.at(i).render_semaphore);
		VK_DESTROY(sync_structs.at(i).render_fence, vkDestroyFence, device, sync_structs.at(i).render_fence);
	}

	cleanup_queue.del_all_obj();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Engine::run()
{
	camera_controller.init_time();

	while (!glfwWindowShouldClose(window)) {
		update();

		if (aquire_image()) {
			draw();

			present();

			late_update();
		}

		if (resize_window) {
			recreate_swapchain();
			resize_window = false;
		}

	}

	vkDeviceWaitIdle(device);
}


void Engine::update()
{
	glfwPollEvents();

	camera_controller.update_time();
	camera_controller.handle_keys();

	gui_input = gui.get_input();

	camera_controller.set_move_strength(gui_input.camera_movement_speed);
	camera_controller.set_rotation_strength(gui_input.camera_rotation_speed);

	terrain.set_tesselation_strength(gui_input.terrain_tesselation);
	terrain.set_max_tesselation(gui_input.max_terrain_tesselation);
	terrain.set_height_scale(gui_input.terrain_height_scale);
	terrain.set_texture_eps(gui_input.terrain_texture_eps);
	terrain.set_visualization_mode(gui_input.terrain_vis_mode);

	update_uniforms();
}

void Engine::draw()
{
	const VKW_CommandBuffer& cmd = get_current_command_buffer();
	cmd.reset();

	// begin command buffer
	cmd.begin();
	
	{
		// initial layout transitions
		Texture::transition_layout(cmd, color_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		Texture::transition_layout(cmd, depth_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		// draw environment map
		{
			
			VKW_GraphicsPipeline& pipeline = environment_map_pipeline;
			pipeline.set_render_size(swapchain.get_extent());

			// would theoretically not need to clear the screen
			pipeline.set_color_attachment(
				color_render_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
				true,
				{ {0.2f, 0.2f, 0.2f, 1.0f} }
			);

			pipeline.set_depth_attachment(
				depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
				true,
				1.0f
			);
			
			pipeline.begin_rendering(cmd);
			{
				pipeline.bind(cmd);

				pipeline.set_dynamic_viewport(cmd);
				pipeline.set_dynamic_scissor(cmd);
				environment_map.draw(cmd, current_frame, pipeline);
			}
			pipeline.end_rendering(cmd);
		}
		

		// draw terrain
		{
			VKW_GraphicsPipeline& pipeline = (gui_input.terrain_wireframe_mode) ? terrain_wireframe_pipeline : terrain_pipeline;
			pipeline.set_render_size(swapchain.get_extent());

			pipeline.set_color_attachment(
				color_render_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
				false,
				{ {1.0f, 0.0f, 1.0f, 1.0f} }
			);

			pipeline.set_depth_attachment(
				depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
				false,
				1.0f
			);

			pipeline.begin_rendering(cmd);
			{
				pipeline.bind(cmd);

				pipeline.set_dynamic_viewport(cmd);
				pipeline.set_dynamic_scissor(cmd);
				terrain.draw(cmd, current_frame, pipeline);
			}
			pipeline.end_rendering(cmd);
		}
		
		// transitions for copy into swapchain images
		Texture::transition_layout(cmd, color_render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy (both have swapchain extent resolution
		Texture::copy(cmd, color_render_target, swapchain.images_at(current_swapchain_image_idx), swapchain.get_extent(), swapchain.get_extent());

		
		// transition for imgui
		Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// draw imgui
		gui.draw(cmd, current_swapchain_image_idx);


		// transition for present
		Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}
	
	// end and submit command buffer

	cmd.submit(
		{ get_current_swapchain_semaphore() },
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
		{ get_current_render_semaphore() },
		get_current_render_fence()
	);
	
}

void Engine::present()
{
	if (!swapchain.present({ get_current_render_semaphore() }, current_swapchain_image_idx)) {
		std::cout << "Recreate swapchain (Present)" << std::endl;
		resize_window = true;
	}
}

void Engine::late_update()
{
	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

}

inline void Engine::resize()
{
}

// TODO: Query glfwGetRequiredInstanceExtensions() and add it to list of required extensions
void Engine::init_glfw()
{
	if (!glfwInit()) {
		throw SetupException("GLFW Initilization failed", __FILE__, __LINE__);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(res_x, res_y, "Wulkan", nullptr, nullptr);

	if (!window) {
		glfwTerminate();
		throw SetupException("GLFW Window creation failed", __FILE__, __LINE__);
	}

	glfwSetWindowUserPointer(window, this);
	
	// disable cursor so can use raw mouse movement
	if (!glfwRawMouseMotionSupported()) {
		throw std::runtime_error("Raw mouse motion not supported");
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, glfm_mouse_move_callback);
}

void Engine::init_vulkan()
{
	VK_CHECK_ET(volkInitialize(), RuntimeException, "Failed to initialize volk");

	init_instance();
	create_surface();
	create_device();
	create_queues();

	create_swapchain();
	
	create_command_structs();
	create_sync_structs();

	create_texture_samplers();

	create_uniform_buffers();

	init_shared_data();
	create_descriptor_sets();

	init_data();


	create_graphics_pipelines();
}

void Engine::init_instance()
{
	instance.init("Wulkan", get_required_instance_extensions(), std::vector<const char*>());
	cleanup_queue.add(&instance);
}

void Engine::create_surface()
{
	surface.init(window, &instance);
	cleanup_queue.add(&surface);
}

void Engine::create_device()
{
	Required_Device_Features required_features = get_required_device_features();

	device.init(&instance, surface, get_required_device_extensions(), required_features, "Device");
	cleanup_queue.add(&device);

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device.get_physical_device(), &deviceProperties);
	std::cout << "Selected GPU:" << deviceProperties.deviceName << std::endl;
	std::cout << "Tesselation limit:" << deviceProperties.limits.maxTessellationGenerationLevel << std::endl;
}

void Engine::create_queues()
{
	graphics_queue.init(device, vkb::QueueType::graphics, "Graphics queue");
	present_queue.init(device, vkb::QueueType::present, "Present queue");
	transfer_queue.init(device, vkb::QueueType::transfer, "Transfer queue");
	std::cout << "Chosen queues:" << graphics_queue.get_queue_family() << "," << present_queue.get_queue_family() << "," << transfer_queue.get_queue_family() << std::endl;
}

void Engine::init_shared_data()
{
	shared_terrain_data.init(&device);
	cleanup_queue.add(&shared_terrain_data);

	shared_environment_data.init(&device);
	cleanup_queue.add(&shared_environment_data);
}

void Engine::init_data()
{
	terrain.init(
		device,
		get_current_graphics_pool(),
		get_current_transfer_pool(),
		descriptor_pool,
		&mirror_texture_sampler,
		&shared_terrain_data,

		"textures/terrain/heightmap.png",
		//"textures/terrain/test/height_test4.png",
		"textures/terrain/texture.png",
		"textures/terrain/normal.png",
		256							// resolution of mesh
	);
	terrain.set_descriptor_bindings(uniform_buffers);
	cleanup_queue.add(&terrain);

	environment_map.init(
		device,
		get_current_graphics_pool(),
		get_current_transfer_pool(),
		descriptor_pool,
		&shared_environment_data,

		"textures/environment_maps/day_cube_map_%.exr"
		//"textures/environment_maps/rogland_clear_night_%.exr"
		//"textures/environment_maps/test/test_%.exr"
	);
	environment_map.set_descriptor_bindings(uniform_buffers, linear_texture_sampler);
	cleanup_queue.add(&environment_map);
}

void Engine::create_texture_samplers()
{
	linear_texture_sampler.init(&device, "Default sampler");
	cleanup_queue.add(&linear_texture_sampler);

	mirror_texture_sampler.set_address_mode(VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
	mirror_texture_sampler.init(&device, "Repeat sampler");
	cleanup_queue.add(&mirror_texture_sampler);
}

void Engine::create_descriptor_sets()
{
	imgui_descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
	imgui_descriptor_pool.init(&device, 1, "Imgui descriptor pool");
	cleanup_queue.add(&imgui_descriptor_pool);
	
	// each Terrain instance needs MAX_FRAMES_IN_FLIGHT many descriptor sets
	descriptor_pool.add_layout(shared_terrain_data.get_descriptor_set_layout(), MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_layout(shared_environment_data.get_descriptor_set_layout(), MAX_FRAMES_IN_FLIGHT);

	descriptor_pool.init(&device, MAX_FRAMES_IN_FLIGHT*2, "General descriptor pool");
	cleanup_queue.add(&descriptor_pool);
}

void Engine::create_uniform_buffers()
{
	for (VKW_Buffer& uniform_buffer : uniform_buffers) {
		uniform_buffer.init(
			&device, 
			sizeof(UniformStruct), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			sharing_exlusive(), 
			true,
			"Uniform buffer"
		);
		uniform_buffer.map();
		cleanup_queue.add(&uniform_buffer);
	}
}

void Engine::init_render_targets()
{
	color_render_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_Colortarget),
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		sharing_exlusive(),
		"Color render target"
	); // TODO: FIX might add it multiple times to the clean up queue	
	cleanup_queue.add(&color_render_target);

	depth_render_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_D),
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		sharing_exlusive(),
		"Depth render target"
	);
	cleanup_queue.add(&depth_render_target);
}

void Engine::create_graphics_pipelines()
{
	terrain_pipeline = Terrain::create_pipeline(&device, color_render_target, depth_render_target, shared_terrain_data);
	cleanup_queue.add(&terrain_pipeline);

	terrain_wireframe_pipeline = Terrain::create_pipeline(&device, color_render_target, depth_render_target, shared_terrain_data, true);
	cleanup_queue.add(&terrain_wireframe_pipeline);

	environment_map_pipeline = EnvironmentMap::create_pipeline(&device, color_render_target, depth_render_target, shared_environment_data);
	cleanup_queue.add(&environment_map_pipeline);
}

void Engine::create_swapchain()
{
	swapchain.init(window, device, &present_queue, "Swapchain");
	cleanup_queue.add(&swapchain);

	init_render_targets();
}

void Engine::recreate_swapchain()
{
	// dont recreate swapchain while image is minimized
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	res_x = width;
	res_y = height;

	vkDeviceWaitIdle(device);

	swapchain.recreate(window, device);

	recreate_render_targets();

	camera.set_aspect_ratio(res_x, res_y);
}

void Engine::recreate_render_targets()
{
	// Destroys render targets

	color_render_target.del();
	depth_render_target.del();

	// important to clear i.e. image view cache
	color_render_target = {};
	depth_render_target = {};

	init_render_targets();
}

void Engine::create_command_structs()
{
	// Use 1 command pool with one buffer per thread and image in flight
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		command_structs.at(i) = {
			{},
			{},
			{},
		};

		command_structs.at(i).graphics_command_pool.init(&device, &graphics_queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, "Graphics pool");
		cleanup_queue.add(&command_structs.at(i).graphics_command_pool);
		
		command_structs.at(i).transfer_command_pool.init(&device, &transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, "Transfer pool");
		cleanup_queue.add(&command_structs.at(i).transfer_command_pool);
		
		command_structs.at(i).graphics_command_buffer.init(&device, &command_structs.at(i).graphics_command_pool, false, "Draw CMD");
	}
}

void Engine::create_sync_structs()
{
	// TODO: look into Semaphore/Fence pool
	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start in signaled state

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		sync_structs.at(i) = {};
		VK_CHECK_E(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &sync_structs.at(i).swapchain_semaphore), SetupException);
		VK_CHECK_E(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &sync_structs.at(i).render_semaphore), SetupException);
		VK_CHECK_E(vkCreateFence(device, &fence_create_info, nullptr, &sync_structs.at(i).render_fence), SetupException);
	}
}

bool Engine::aquire_image()
{
	VkFence render_fence = get_current_render_fence();
	VK_CHECK_E(vkWaitForFences(device, 1, &render_fence, VK_TRUE, UINT64_MAX), RuntimeException);
	
	VkResult aquire_image_result = vkAcquireNextImageKHR(
		device, 
		swapchain,
		UINT64_MAX, 
		get_current_swapchain_semaphore(),
		VK_NULL_HANDLE, 
		&current_swapchain_image_idx
	);
	if (aquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
		std::cout << "Recreate swapchain (Aquire)" << std::endl;
		resize_window = true;

		return false;
	}
	else if (aquire_image_result != VK_SUCCESS && aquire_image_result != VK_SUBOPTIMAL_KHR) {
		throw RuntimeException("Failed to aquire image from swapchain: " + std::string(string_VkResult(aquire_image_result)), __FILE__, __LINE__);
	}
	
	// we have successfully aquired image, can reset fence
	VK_CHECK_E(vkResetFences(device, 1, &render_fence), RuntimeException);

	return true;
}

void Engine::update_uniforms()
{
	UniformStruct uniform{};
	uniform.proj = camera.generate_projection_mat();
	uniform.proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication

	uniform.view = camera.generate_view_mat();
	uniform.inv_view = glm::inverse(uniform.view);
	uniform.virtual_view = camera.generate_virtual_view_mat();

	uniform.near_far_plane = glm::vec2(camera.get_near_plane(), camera.get_far_plane());
	uniform.sun_direction = glm::normalize(gui_input.sun_direction);

	memcpy(uniform_buffers.at(current_frame).get_mapped_address(), &uniform, sizeof(UniformStruct));
}

std::vector<const char*> Engine::get_required_instance_extensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.push_back("VK_EXT_debug_utils");

	return extensions;
}

std::vector<const char*> Engine::get_required_device_extensions()
{
	// VK_KHR_SWAPCHAIN_EXTENSION_NAME is added automatically
	std::vector<const char*> extensions{};
	return extensions;
}

Required_Device_Features Engine::get_required_device_features()
{
	Required_Device_Features features{};
	// 1.0 features
	features.rf.samplerAnisotropy = true;
	features.rf.fillModeNonSolid = true;
	features.rf.tessellationShader = true;
	// 1.2 features
	features.rf12.bufferDeviceAddress = true;
	features.rf12.descriptorIndexing = true;
	// 1.3 features
	features.rf13.dynamicRendering = true;
	features.rf13.synchronization2 = true;

	return features;
}

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y) {
	Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (engine) {
		engine->camera_controller.handle_mouse(pos_x, pos_y);
	}
	else {
		throw SetupException("GLFW Engine User pointer not set", __FILE__, __LINE__);
	}
}