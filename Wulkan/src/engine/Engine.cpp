#include "Engine.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <GLFW/glfw3.h>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp> // todo remove

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
	terrain.set_model_matrix(
		glm::scale(glm::mat4(1), glm::vec3(25.0, 25.0, 25.0 * gui_input.terrain_height_scale))
	);
	terrain.set_texture_eps(gui_input.terrain_texture_eps);
	terrain.set_visualization_mode(gui_input.terrain_vis_mode);

	directional_light.set_direction(glm::normalize(gui_input.sun_direction));

	update_uniforms();
}

void Engine::draw()
{
	{
		ShadowDepthOnlyUniformData uniform{};
		glm::mat4 proj = directional_light.shadow_camera.generate_ortho_mat();
		proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication

		float near_plane = camera.get_near_plane();
		float far_plane = camera.get_far_plane();
		float lambda = 0.5;

		int N = 4;
		float zero_one_splits[5] = {}; // split planes (inluding near plane) normalized to 0 to 1
		zero_one_splits[0] = 0;

		// formula from original paper
		for (int i = 1; i <= N; i++) {
			float ratio = ((float)i) / N;
			uniform.split_planes[i-1] = lambda * near_plane * powf(far_plane / near_plane, ratio)
									+ (1 - lambda) * (near_plane + ratio * (far_plane - near_plane));
			
			float d = uniform.split_planes[i - 1];
			// inverted the function to linearize depth as this is already linear atm.
			zero_one_splits[i] = (near_plane * far_plane / d - far_plane) / (near_plane - far_plane);
		}

				
		glm::mat4 camera_proj = camera.generate_projection_mat();
		camera_proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication

		glm::mat4 camera_view = camera.generate_virtual_view_mat();

		glm::mat4 inv_proj_camera = glm::inverse(camera_proj);
		glm::mat4 inv_view_camera = glm::inverse(camera_view);
		glm::mat4 inv_proj_view_camera = glm::inverse(camera_proj * camera_view);
		
		glm::mat4 light_view = directional_light.shadow_camera.generate_view_mat();

		for (int cascade_idx = 0; cascade_idx < N; cascade_idx++) {
			// compute corner points of frustum p
			glm::vec3 frustumNDC[8] = {
				glm::vec3(-1.0f,  1.0f, zero_one_splits[cascade_idx]),
				glm::vec3(1.0f,  1.0f,  zero_one_splits[cascade_idx]),
				glm::vec3(1.0f, -1.0f,  zero_one_splits[cascade_idx]),
				glm::vec3(-1.0f, -1.0f,  zero_one_splits[cascade_idx]),
				glm::vec3(-1.0f,  1.0f,  zero_one_splits[cascade_idx+1]),
				glm::vec3(1.0f,  1.0f,   zero_one_splits[cascade_idx + 1]),
				glm::vec3(1.0f, -1.0f,   zero_one_splits[cascade_idx + 1]),
				glm::vec3(-1.0f, -1.0f,   zero_one_splits[cascade_idx + 1]),
			};

			glm::vec3 frustum_light[8] = {};
			glm::vec3 min_v = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 max_v = glm::vec3(std::numeric_limits<float>::min());

			std::vector<glm::vec3> debug_frustum{};
			for (int i = 0; i < 8; i++) {				
				glm::vec4 frustum_world_space = inv_proj_view_camera * glm::vec4(frustumNDC[i], 1);
				frustum_world_space /= frustum_world_space.w;

				debug_frustum.push_back(glm::vec3(frustum_world_space));
				

				glm::vec4 frustum_light_view = proj * light_view * frustum_world_space;
				frustum_light_view /= frustum_light_view.w;

				frustum_light[i] = glm::vec3(frustum_light_view);

				min_v = glm::min(min_v, frustum_light[i]);
				max_v = glm::max(max_v, frustum_light[i]);
			}
			debug_lines.at(cascade_idx).update_vertices(debug_frustum);

			//glm::mat4 P_z = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near_plane, max_v.z);
			
			light_frustum.set_camera_matrix(proj * light_view);
		}



		for (uint32_t i = 0; i < 4; i++) {
			uniform.proj_view[i] = proj * light_view;
		}

		memcpy(directional_light.uniform_buffers.at(current_frame).get_mapped_address(), &uniform, sizeof(ShadowDepthOnlyUniformData));


		const VKW_CommandBuffer& shadow_cmd = directional_light.cmds.at(current_frame);
		shadow_cmd.reset();
		shadow_cmd.begin();
		

		{
			Texture::transition_layout(shadow_cmd, directional_light.depth_rt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			// draw using depth only pipelines
			{
				VKW_GraphicsPipeline& pipeline = terrain_depth_pipeline;
				pipeline.set_render_size(directional_light.depth_rt.get_extent());

				for (uint32_t i = 0; i < 4; i++) {
					pipeline.set_depth_attachment(
						directional_light.depth_rt.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, i, 1),
						true,
						1.0f
					);

					pipeline.begin_rendering(shadow_cmd);
					{
						pipeline.bind(shadow_cmd);
						pipeline.set_dynamic_state(shadow_cmd);
						pipeline.set_depth_bias(gui_input.depth_bias, gui_input.slope_depth_bias, shadow_cmd);

						terrain.set_cascade_idx(i);
						terrain.draw(shadow_cmd, current_frame, pipeline);
					}
					pipeline.end_rendering(shadow_cmd);
				}
			}


			Texture::transition_layout(shadow_cmd, directional_light.depth_rt, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		}

		shadow_cmd.submit(
			{ },
			{ },
			{ directional_light.shadow_semaphores.at(current_frame) },
			VK_NULL_HANDLE
		);
	}

	{

		const VKW_CommandBuffer& cmd = get_current_command_buffer();
		cmd.reset(); // resetting command pool might be more efficient

		// begin command buffer
		cmd.begin();
	
		{
			// initial layout transitions
			Texture::transition_layout(cmd, color_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			Texture::transition_layout(cmd, depth_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL); // TODO check if necessary for depth each frame

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

					pipeline.set_dynamic_state(cmd);
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

					pipeline.set_dynamic_state(cmd);
					
					terrain.draw(cmd, current_frame, pipeline);
				}
				pipeline.end_rendering(cmd);
			}

			// draw lines
			{
				VKW_GraphicsPipeline& pipeline = line_pipeline;
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

					pipeline.set_dynamic_state(cmd);

					for (int i = 0; i < 4; i++) {
						debug_lines.at(i).draw(cmd, current_frame, pipeline);
					}
					light_frustum.draw(cmd, current_frame, pipeline);
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
			{ get_current_swapchain_semaphore(), directional_light.shadow_semaphores.at(current_frame) },
			{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },
			{ get_current_render_semaphore() },
			get_current_render_fence()
		);
	}
	
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

	shared_line_data.init(&device);
	cleanup_queue.add(&shared_line_data);
}

void Engine::init_data()
{
	std::array<VKW_CommandPool, MAX_FRAMES_IN_FLIGHT> graphic_pools;
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		graphic_pools.at(i) = command_structs.at(i).graphics_command_pool;
	}
	directional_light.init(
		&device,
		graphic_pools,
		
		glm::vec3(0, 0, 0), // look at for shadow
		glm::vec3(0, 0, 1), // direction for shadow
		30, // distance from look at for projection
		4*1024,
		4*1024,
		40, // height of orthographic projection
		0.1,
		50.0
	);
	directional_light.set_color(glm::vec3(0.8, 0.8, 1.0));
	directional_light.set_intensity(1.0f);
	//directional_light.init(glm::vec3(0, 0, 1), glm::vec3(0.8, 0.8, 1), 1.0);
	cleanup_queue.add(&directional_light);

	// terrain needs directional light to be initiated due to it relying on it's uniform buffer
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
	terrain.set_descriptor_bindings(uniform_buffers, directional_light.uniform_buffers, directional_light.depth_rt, linear_texture_sampler);
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

	std::array<glm::vec4, 4> colors = {
		glm::vec4(1,0,0,1),
		glm::vec4(1,1,0,1),
		glm::vec4(0,1,0,1),
		glm::vec4(0,0,1,1)
	};
	for (int i = 0; i < 4; i++) {
		Line& line = debug_lines.at(i);
		line.init(
			device,
			get_current_transfer_pool(),
			descriptor_pool,
			&shared_line_data,
			{ 
				glm::vec3(0,0,0), glm::vec3(1, 0, 0),  glm::vec3(1, 1, 0),  glm::vec3(0, 1, 0),
				glm::vec3(0,0,1), glm::vec3(1, 0, 1),  glm::vec3(1, 1, 1),  glm::vec3(0, 1, 1) 
			},
			{
				0, 1, 1, 2, 2, 3, 3, 0,
				0, 4, 1, 5, 2, 6, 3, 7,
				4, 5, 5, 6, 6, 7, 7, 4
			},
			colors.at(i)
		);
		line.set_descriptor_bindings(uniform_buffers);
		line.set_model_matrix(
			glm::mat4(1)
		);
		cleanup_queue.add(&line);
	}

	light_frustum.init(
		device,
		get_current_transfer_pool(),
		descriptor_pool,
		&shared_line_data,
		glm::mat4(1),
		glm::vec4(1, 0, 1, 1)
	);
	light_frustum.set_descriptor_bindings(uniform_buffers);
	light_frustum.set_model_matrix(
		glm::mat4(1)
	);
	cleanup_queue.add(&light_frustum);
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
	descriptor_pool.add_layout(shared_line_data.get_descriptor_set_layout(), MAX_FRAMES_IN_FLIGHT*5);

	descriptor_pool.init(&device, MAX_FRAMES_IN_FLIGHT*7, "General descriptor pool");
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
			"Main Uniform buffer"
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

	terrain_wireframe_pipeline = Terrain::create_pipeline(&device, color_render_target, depth_render_target, shared_terrain_data, false, true); // wireframe but not depth only
	cleanup_queue.add(&terrain_wireframe_pipeline);

	terrain_depth_pipeline = Terrain::create_pipeline(&device, color_render_target, depth_render_target, shared_terrain_data, true, false, true);
	cleanup_queue.add(&terrain_depth_pipeline);

	environment_map_pipeline = EnvironmentMap::create_pipeline(&device, color_render_target, depth_render_target, shared_environment_data);
	cleanup_queue.add(&environment_map_pipeline);

	line_pipeline = Line::create_pipeline(&device, color_render_target, depth_render_target, shared_line_data);
	cleanup_queue.add(&line_pipeline);
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

		device.name_object((uint64_t)sync_structs.at(i).swapchain_semaphore, VK_OBJECT_TYPE_SEMAPHORE, "Swapchain semaphore");
		device.name_object((uint64_t)sync_structs.at(i).render_semaphore, VK_OBJECT_TYPE_SEMAPHORE, "Render semaphore");
		device.name_object((uint64_t)sync_structs.at(i).render_fence, VK_OBJECT_TYPE_FENCE, "Render fence");
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
	
	uniform.sun_direction = directional_light.get_direction();
	uniform.sun_color = directional_light.get_color();
	
	{
		glm::mat4 proj = directional_light.shadow_camera.generate_ortho_mat();
		proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication
		glm::mat4 view = directional_light.shadow_camera.generate_view_mat();
		uniform.sun_proj_view = proj * view;
	}
	
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