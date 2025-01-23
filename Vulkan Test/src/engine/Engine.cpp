#include "Engine.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <glm/gtc/matrix_transform.hpp>


#include <GLFW/glfw3.h>

#include <iostream>

#ifdef NDEBUG
#else
#endif

void Engine::init(unsigned int w, unsigned int h)
{
	res_x = w;
	res_y = h;

	init_glfw();
	init_vulkan();

	camera = Camera( glm::vec3(0.0, 12.0, 8.0), glm::vec3(0.0, 0.0, 8.0), res_x, res_y, glm::radians(45.0f), 0.01f, 100.0f );
	camera_controller = CameraController(window, &camera);
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

		draw();

		present();

		late_update();
	}
}


void Engine::update()
{
	glfwPollEvents();

	camera_controller.update_time();
	camera_controller.handle_keys();

	update_uniforms();
}

void Engine::draw()
{
	aquire_image();
	get_current_command_buffer().reset();

	// begin command buffer
	{
	}
	// end command buffer

	/*
	get_current_command_buffer().submit(
		{ get_current_swapchain_semaphore() },
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
		{ get_current_render_semaphore() },
		get_current_render_fence()
	);
	*/
}

void Engine::present()
{
	if (!swapchain.present({ get_current_render_semaphore() }, current_swapchain_image_idx)) {
		std::cout << "Recreate swapchain (Present)" << std::endl;

		// recreate_swapchain(); 
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

	window = glfwCreateWindow(res_x, res_y, "Vulkan", nullptr, nullptr);

	if (!window) {
		glfwTerminate();
		throw SetupException("GLFW Window creation failed", __FILE__, __LINE__);
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, glfw_window_resize_callback);

	// disable cursor so can use raw mouse movement
	if (!glfwRawMouseMotionSupported()) {
		throw std::runtime_error("Raw mouse motion not supported");
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, glfm_mouse_move_callback);
}

void Engine::init_vulkan()
{
	init_instance();
	create_surface();
	create_device();
	create_queues();

	create_swapchain();
	
	create_command_structs();
	create_sync_structs();

	create_texture_samplers();

	create_uniform_buffers();
	create_descriptor_sets();

	init_terrain_data();


	create_graphics_pipelines();
}

void Engine::init_instance()
{
	instance.init("VK Study", get_required_instance_extensions(), std::vector<const char*>());
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

	device.init(&instance, surface, get_required_device_extensions(), required_features);;
	cleanup_queue.add(&device);

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device.get_physical_device(), &deviceProperties);
	std::cout << "Selected GPU:" << deviceProperties.deviceName << std::endl;
	std::cout << "Tesselation limit:" << deviceProperties.limits.maxTessellationGenerationLevel << std::endl;
}

void Engine::create_queues()
{
	graphics_queue.init(device, vkb::QueueType::graphics);
	present_queue.init(device, vkb::QueueType::present);
	transfer_queue.init(device, vkb::QueueType::transfer);
	std::cout << "Chosen queues:" << graphics_queue.get_queue_family() << "," << present_queue.get_queue_family() << "," << transfer_queue.get_queue_family() << std::endl;
}

void Engine::init_terrain_data()
{
	terrain.init(
		device,
		get_current_graphics_pool(),
		get_current_transfer_pool(),
		&descriptor_pool,
		&shared_terrain_data,

		"textures/perlinNoise.png",
		32							// resolution of mesh
	);
	terrain.set_descriptor_bindings(uniform_buffers, linear_texture_sampler);
	cleanup_queue.add(&terrain);
	/*
	terrain_height_map = create_texture_from_path(
		&device,
		&get_current_graphics_pool(),
		"textures/perlinNoise.png",
		Texture_Type::Tex_R
	);
	cleanup_queue.add(&terrain_height_map);


	const uint32_t mesh_res = 32;
	std::vector<Vertex> terrain_vertices{};
	std::vector<uint32_t> terrain_indices{};

	for (uint32_t iy = 0; iy < mesh_res; iy++) {
		for (uint32_t ix = 0; ix < mesh_res; ix++) {
			float u = ((float)ix) / mesh_res;
			float v = ((float)iy) / mesh_res;
			float pos_x = (u - 0.5) * 2;
			float pos_y = (v - 0.5) * 2;

			terrain_vertices.push_back({ {pos_x,pos_y,0}, u, {0,0,1}, v, {1,0,0,1} });
		}
	}

	for (uint32_t iy = 0; iy < mesh_res - 1; iy++) {
		for (uint32_t ix = 0; ix < mesh_res - 1; ix++) {
			terrain_indices.push_back(ix + iy * mesh_res);
			terrain_indices.push_back(ix + 1 + iy * mesh_res);
			terrain_indices.push_back(ix + 1 + (iy + 1) * mesh_res);
			terrain_indices.push_back(ix + (iy + 1) * mesh_res);
		}
	}

	terrain_mesh.init(device, get_current_transfer_pool(), terrain_vertices, terrain_indices);
	cleanup_queue.add(&terrain_mesh);
	*/
}

void Engine::create_texture_samplers()
{
	linear_texture_sampler.init(&device);
	cleanup_queue.add(&linear_texture_sampler);
}

void Engine::create_descriptor_sets()
{
	imgui_descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
	imgui_descriptor_pool.init(&device, 1);
	cleanup_queue.add(&imgui_descriptor_pool);

	
	shared_terrain_data.init(&device);
	cleanup_queue.add(&shared_terrain_data);
	
	// each Terrain instance needs MAX_FRAMES_IN_FLIGHT many descriptor sets
	descriptor_pool.add_layout(shared_terrain_data.get_descriptor_set_layout(), MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.init(&device, MAX_FRAMES_IN_FLIGHT);
	cleanup_queue.add(&descriptor_pool);

	/*
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VKW_DescriptorSet& set = descriptor_sets.at(i);
		set.init(&device, &descriptor_pool, descriptor_set_layout);

		// set the bindings
		set.update(0, uniform_buffers.at(i));
		set.update(1, terrain_height_map, linear_texture_sampler, VK_IMAGE_ASPECT_COLOR_BIT);
		
		cleanup_queue.add(&set);
	}
	*/

	//terrain_vertex_push_constants.init(VK_SHADER_STAGE_VERTEX_BIT);
	//terrain_push_constants.init(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
}

void Engine::create_uniform_buffers()
{
	for (VKW_Buffer& uniform_buffer : uniform_buffers) {
		uniform_buffer.init(
			&device, 
			sizeof(UniformStruct), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			sharing_exlusive(), 
			true
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
		sharing_exlusive()
	); // TODO: FIX might add it multiple times to the clean up queue
	cleanup_queue.add(&color_render_target);

	depth_render_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_D),
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		sharing_exlusive()
	);
	cleanup_queue.add(&depth_render_target);
}

void Engine::create_graphics_pipelines()
{
	terrain_pipeline = Terrain::create_pipeline(&device, color_render_target, depth_render_target, shared_terrain_data);
	cleanup_queue.add(&terrain_pipeline);
	/*
	VKW_Shader terrain_vert_shader{};
	terrain_vert_shader.init(&device, "shaders/terrain_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	VKW_Shader terrain_frag_shader{};
	terrain_frag_shader.init(&device, "shaders/terrain_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VKW_Shader tess_ctrl_shader{};
	tess_ctrl_shader.init(&device, "shaders/terrain_tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

	VKW_Shader tess_eval_shader{};
	tess_eval_shader.init(&device, "shaders/terrain_tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

	terrain_pipeline.set_topology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	terrain_pipeline.set_culling_mode(VK_CULL_MODE_NONE); // TODO backface culling
	terrain_pipeline.enable_depth_test();
	terrain_pipeline.enable_depth_write();

	terrain_pipeline.enable_tesselation();
	terrain_pipeline.set_tesselation_patch_size(4);

	terrain_pipeline.add_shader_stages({ terrain_vert_shader, terrain_frag_shader, tess_ctrl_shader, tess_eval_shader });
	terrain_pipeline.add_descriptor_sets({ shared_terrain_data.get_descriptor_set_layout()});

	terrain_pipeline.add_push_constants(shared_terrain_data.get_push_consts_range());

	terrain_pipeline.set_color_attachment_format(color_render_target.get_format());
	terrain_pipeline.set_depth_attachment_format(depth_render_target.get_format());

	terrain_pipeline.set_color_attachment(
		color_render_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
		true,
		{ {0.0f, 0.0f, 0.0f, 1.0f} }
	);

	terrain_pipeline.set_depth_attachment(
		depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
		true,
		1.0f
	);

	terrain_pipeline.init(&device);
	cleanup_queue.add(&terrain_pipeline);

	terrain_vert_shader.del();
	terrain_frag_shader.del();
	tess_ctrl_shader.del();
	tess_eval_shader.del();
	*/
}

void Engine::create_swapchain()
{
	swapchain.init(window, device, &present_queue);
	cleanup_queue.add(&swapchain);

	init_render_targets();
}

void Engine::recreate_swapchain()
{
  // TODO wait device idle
	destroy_swapchain();

	// TODO swap to recreating using previous swapchain
	create_swapchain();
}

void Engine::destroy_swapchain()
{
	// Destroys images, image views and framebuffers associated with the swapchain
	// TODO: reset images etc.

	swapchain.del();
	color_render_target.del();
	depth_render_target.del();
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

		command_structs.at(i).graphics_command_pool.init(&device, &graphics_queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		cleanup_queue.add(&command_structs.at(i).graphics_command_pool);
		
		command_structs.at(i).transfer_command_pool.init(&device, &transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		cleanup_queue.add(&command_structs.at(i).transfer_command_pool);
		
		command_structs.at(i).graphics_command_buffer.init(&device, &command_structs.at(i).graphics_command_pool, false);
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

void Engine::aquire_image()
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
	if (aquire_image_result == VK_ERROR_OUT_OF_DATE_KHR || resize_window) {
		std::cout << "Recreate swapchain (Aquire)" << std::endl;
		// resize_window = false;
		// recreate_swapchain(); 
		// return;
	}
	else if (aquire_image_result != VK_SUCCESS && aquire_image_result != VK_SUBOPTIMAL_KHR) {
		throw RuntimeException("Failed to aquire image from swapchain: " + std::string(string_VkResult(aquire_image_result)), __FILE__, __LINE__);
	}
	
	// we have successfully aquired image, can reset fence
	VK_CHECK_E(vkResetFences(device, 1, &render_fence), RuntimeException);
}

void Engine::update_uniforms()
{
	UniformStruct uniform{};
	uniform.proj = camera.generate_projection_mat();
	uniform.proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication

	uniform.view = camera.generate_view_mat();
	uniform.virtual_view = camera.generate_virtual_view_mat();

	memcpy(uniform_buffers.at(current_frame).get_mapped_address(), &uniform, sizeof(UniformStruct));
}

std::vector<const char*> Engine::get_required_instance_extensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

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

void glfw_window_resize_callback(GLFWwindow* window, int width, int height) {
	Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (engine) {
		engine->resize_callback(width, height);
	}
	else {
		throw SetupException("GLFW Engine User pointer not set", __FILE__, __LINE__);
	}
}
