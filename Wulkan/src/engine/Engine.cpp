#include "common.h"
#include "Engine.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <GLFW/glfw3.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <cstdio>

#include <random>

void Engine::init(unsigned int w, unsigned int h)
{
	res_x = w;
	res_y = h;

	init_logger();
#ifndef NDEBUG
		spdlog::info("Debugging");
#endif

	init_glfw();
	init_vulkan();

	camera = Camera( glm::vec3(0.0, 60.0, 25.0), glm::vec3(0.0, 10.0, 8.0), res_x, res_y, glm::radians(45.0f), 0.1f, 100.0f );
	camera_controller = CameraController(window, &camera);
	gui.init(window, instance, device, graphics_queue, imgui_descriptor_pool, &swapchain, &camera_controller, &glfw_input_mutex);
	cleanup_queue.add(&gui);
 }

Engine::~Engine()
{
	render_thread.join();

	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_DESTROY(sync_structs.at(i).swapchain_semaphore, vkDestroySemaphore, device, sync_structs.at(i).swapchain_semaphore);
		VK_DESTROY(sync_structs.at(i).render_semaphore, vkDestroySemaphore, device, sync_structs.at(i).render_semaphore);
		VK_DESTROY(sync_structs.at(i).render_fence, vkDestroyFence, device, sync_structs.at(i).render_fence);

		TracyVkDestroy(command_structs.at(i).graphics_queue_tracy_context);
	}

	
	cleanup_queue.del_all_obj();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Engine::run()
{
	tracy::SetThreadName("Main thread (IO)");

	camera_controller.init_time();

	render_thread = std::thread(&Engine::render_thread_func, this);

	while (!glfwWindowShouldClose(window)) {
		ZoneScopedN("IO (GLFW)");

		{
			ZoneScopedN("GLFW Poll");
			glfwPollEvents();
		}

		glfw_input_mutex.lock();
		camera_controller.update_time();
		camera_controller.handle_keys(); // needs to be called by main thread
		glfw_input_mutex.unlock();
	}
	
	spdlog::info("Close window");
	should_window_close.store(true);
}
	

void Engine::render_thread_func()
{
	tracy::SetThreadName("Render thread");

	spdlog::info("Start rendering");

	try {
		while (!should_window_close.load()) {
			if (aquire_image()) {
				update();

				draw();

				present();

				late_update();
			}

			if (resize_window) {
				recreate_swapchain();
				resize_window = false;
			}

			FrameMark;
		}
	} catch (const std::exception& e) {
		spdlog::error(e.what());
		glfwSetWindowShouldClose(window, true); // this may be called from secondary thread
		return;
	}

	vkDeviceWaitIdle(device);
}

void Engine::update()
{
	ZoneScoped;

	{
		ZoneScopedN("IO");

		gui_input = gui.get_input();
		
		camera_controller.set_move_strength(gui_input.camera_movement_speed);
		camera_controller.set_rotation_strength(gui_input.camera_rotation_speed);
		camera.set_near_plane(gui_input.camera_near_plane);
		camera.set_far_plane(gui_input.camera_far_plane);
	}

	{
		ZoneScopedN("Terrain updates");
		terrain.set_tesselation_strength(gui_input.terrain_tesselation);
		terrain.set_max_tesselation(gui_input.max_terrain_tesselation);
		terrain.set_model_matrix(
			glm::scale(glm::rotate(glm::mat4(1), 0.0f, glm::vec3(1, 0, 0)), glm::vec3(25.0, 25.0, 25.0 * gui_input.terrain_height_scale))
		);
		terrain.set_texture_eps(gui_input.terrain_texture_eps);
		terrain.set_visualization_mode(gui_input.terrain_vis_mode);
	}

	{
		ZoneScopedN("Directional light updates");

		directional_light.set_direction(glm::normalize(gui_input.sun_direction));
		directional_light.set_color(gui_input.sun_color);
		directional_light.set_intensity(gui_input.sun_intensity);
		directional_light.set_sample_info(gui_input.receiver_sample_region, gui_input.occluder_sample_region, gui_input.nr_shadow_receiver_samples, gui_input.nr_shadow_occluder_samples);
		directional_light.set_shadow_mode(gui_input.shadow_mode);

		int nr_cascades = gui_input.nr_shadow_cascades;

		glfw_input_mutex.lock();
		directional_light.set_uniforms(camera, nr_cascades, current_frame);
		glfw_input_mutex.unlock();
	}

	{
		ZoneScopedN("Meshes updates");

		meshes[0].set_model_matrix(
			glm::translate(glm::scale(glm::mat4(1), glm::vec3(0.8f)), glm::vec3(10, 0, 25 + cos(glfwGetTime() / 2) / 3))
		);
		meshes[0].set_visualization_mode(gui_input.pbr_vis_mode);

		/*
		meshes[1].set_model_matrix(
			glm::translate(glm::mat4(1), glm::vec3(0,0,40))
		);
		meshes[1].set_visualization_mode(gui_input.pbr_vis_mode);

		meshes[2].set_model_matrix(
			glm::translate(glm::mat4(1), glm::vec3(-5, 0, 30))
		);
		meshes[2].set_visualization_mode(gui_input.pbr_vis_mode);
		*/

		/*
		meshes[3].set_model_matrix(
			glm::translate(
				glm::scale(
					glm::rotate(glm::mat4(1), static_cast<float>(M_PI/2), glm::vec3(1,0,0)),
				glm::vec3(0.01f)),
				glm::vec3(0, 20 * 100,0)
			)
		); // for sponza
		

		meshes[3].set_model_matrix(
			glm::translate(
				glm::scale(
					glm::mat4(1.f),
					glm::vec3(1.f)
				),
				glm::vec3(10, -2.5, 12)
			)
		);
		meshes[3].set_visualization_mode(gui_input.pbr_vis_mode);
		*/

		if (gui_input.draw_trees) {
			lod_mesh.set_camera_info(camera.get_virtual_pos(), camera.get_virtual_dir(), camera.get_near_plane(), camera.get_far_plane());
			lod_mesh.set_visualization_mode(gui_input.pbr_vis_mode);
			lod_mesh.set_lod_ratios(std::vector<float>{gui_input.lod_ratios[0], gui_input.lod_ratios[1], gui_input.lod_ratios[2], 1});

			lod_mesh.update(current_frame);
		}
	}

	update_uniforms();
}

void Engine::draw()
{
	ZoneScoped;

	// shadow pass		
	get_current_graphics_pool().reset();

	{
		// TODO: Use camera controllers active camera
		int nr_cascades = gui_input.nr_shadow_cascades;

		const VKW_CommandBuffer& shadow_cmd = directional_light.begin_depth_pass(current_frame);
		{
			// draw using depth only pipelines
			if (gui_input.shadow_mode != ShadowMode::NoShadows)
			{

				TracyVkZone(get_current_tracy_context(), shadow_cmd, "Shadow [Depth Only]");
				shadow_cmd.begin_debug_zone("Shadow [Depth Only]");
				
				for (int i = 0; i < nr_cascades; i++) {
					{
						TracyVkZone(get_current_tracy_context(), shadow_cmd, "Terrain Depth");

						terrain_depth_render_pass.begin(
							shadow_cmd,
							directional_light.get_texture().get_extent(),
							VK_NULL_HANDLE, // don't attach a color image view
							directional_light.get_texture().get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, i, 1),
							VK_NULL_HANDLE,				// color resolve
							VK_NULL_HANDLE,				// depth resolve
							false,                      // don't clear color
							{ 1,0,1,1 },                // ignore
							true,                       // clear depth
							1.0,		                // clear to far value
							gui_input.depth_bias,       // const depth bias
							gui_input.slope_depth_bias  // slope depth bias
						);

						view_descriptor_sets[current_frame].bind(shadow_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, terrain_depth_render_pass.get_pipeline_layout(), 0);
						shadow_descriptor_sets[current_frame].bind(shadow_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, terrain_depth_render_pass.get_pipeline_layout(), 1);

						terrain.set_cascade_idx(i);
						terrain.draw(shadow_cmd, current_frame);
					
						terrain_depth_render_pass.end(shadow_cmd);
					}

					{
						TracyVkZone(get_current_tracy_context(), shadow_cmd, "PBR Depth");
					
						pbr_depth_pass.begin(
							shadow_cmd,
							directional_light.get_texture().get_extent(),
							VK_NULL_HANDLE, // don't attach a color image view
							directional_light.get_texture().get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, i, 1),
							VK_NULL_HANDLE,				// color resolve
							VK_NULL_HANDLE,				// depth resolve
							false,                      // don't clear color
							{ 1,0,1,1 },                // ignore
							false,                      // clear depth
							0.0,		                // ignore
							gui_input.depth_bias,       // const depth bias
							gui_input.slope_depth_bias  // slope depth bias
						);
						view_descriptor_sets[current_frame].bind(shadow_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_depth_pass.get_pipeline_layout(), 0);
						shadow_descriptor_sets[current_frame].bind(shadow_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_depth_pass.get_pipeline_layout(), 1);

						meshes[0].set_cascade_idx(i);
						meshes[0].draw(shadow_cmd, current_frame);
						/*
						for (size_t j = 0; j < 4; j++) {
							meshes[j].set_cascade_idx(i);
							meshes[j].draw(shadow_cmd, current_frame);
						}
						*/

						if (gui_input.draw_trees) {
							lod_mesh.set_cascade_idx(i);
							lod_mesh.draw(shadow_cmd, current_frame);
						}

						pbr_depth_pass.end(shadow_cmd);
					}
				}

				shadow_cmd.end_debug_zone();
			}
		}
		directional_light.end_depth_pass(current_frame);
	}

	{
		const VKW_CommandBuffer& cmd = get_current_command_buffer();

		// begin command buffer
		cmd.begin();
	
		{
			Texture& color_rt = (use_msaa) ? color_render_target : color_resolve_target;

			// initial layout transitions
			Texture::transition_layout(cmd, color_rt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			Texture::transition_layout(cmd, depth_render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL); // TODO check if necessary for depth each frame

			// draw environment map
			{
				TracyVkZone(get_current_tracy_context(), cmd, "Environment map");
				cmd.begin_debug_zone("Environment map");
				
				environment_render_pass.begin(
					cmd,
					swapchain.get_extent(),
					color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
					depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
					VK_NULL_HANDLE,
					VK_NULL_HANDLE,
					true,                         // color clear
					{ {0.2f, 0.2f, 0.2f, 1.0f} }, // color value
					true,                         // depth clear
					1.0f                          // depth value
				);

				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, environment_render_pass.get_pipeline_layout(), 0);

				environment_map.draw(cmd, current_frame);

				environment_render_pass.end(cmd);

				cmd.end_debug_zone();
			}
			
			// draw terrain
			{
				TracyVkZone(get_current_tracy_context(), cmd, "Terrain");
				cmd.begin_debug_zone("Terrain pass");

				RenderPass<TerrainPushConstants, 3>& render_pass = (gui_input.terrain_wireframe_mode) ? terrain_wireframe_render_passes.at(gui_input.nr_shadow_cascades - 1) : terrain_render_passes.at(gui_input.nr_shadow_cascades - 1);
				render_pass.begin(
					cmd,
					swapchain.get_extent(),
					color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
					depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT)
				);

				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass.get_pipeline_layout(), 0);
				shadow_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass.get_pipeline_layout(), 1);

				terrain.draw(cmd, current_frame);

				render_pass.end(cmd);

				cmd.end_debug_zone();
			}

			// draw meshes
			{
				TracyVkZone(get_current_tracy_context(), cmd, "PBR Meshes");
				cmd.begin_debug_zone("PBR pass");

				pbr_render_pass.begin(
					cmd,
					swapchain.get_extent(),
					color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
					depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT)
				);
				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_render_pass.get_pipeline_layout(), 0);
				shadow_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_render_pass.get_pipeline_layout(), 1);

				meshes[0].draw(cmd, current_frame);
				/*
				for (size_t i = 0; i < 4; i++)
					meshes[i].draw(cmd, current_frame);
				*/

				pbr_render_pass.end(cmd);

				cmd.end_debug_zone();
			}

			{
				TracyVkZone(get_current_tracy_context(), cmd, "PBR Meshes Double sided");
				cmd.begin_debug_zone("PBR pass Double sided");

				pbr_render_double_sided_pass.begin(
					cmd,
					swapchain.get_extent(),
					color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
					depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT)
				);
				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_render_double_sided_pass.get_pipeline_layout(), 0);
				shadow_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_render_double_sided_pass.get_pipeline_layout(), 1);

				if (gui_input.draw_trees) {
					lod_mesh.draw(cmd, current_frame);
				}

				pbr_render_double_sided_pass.end(cmd);

				cmd.end_debug_zone();
			}
			// draw lines (and resolve msaa)
			if (use_msaa) {
				Texture::transition_layout(cmd, color_resolve_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			{
				TracyVkZone(get_current_tracy_context(), cmd, "Debug Lines");
				cmd.begin_debug_zone("Line pass");
				
				line_render_pass.begin(
					cmd,
					swapchain.get_extent(),
					color_rt.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT),
					depth_render_target.get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT),
					use_msaa ? color_resolve_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT) : VK_NULL_HANDLE
				);
				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, line_render_pass.get_pipeline_layout(), 0);
				
				if (gui_input.shadow_draw_debug_frustums)
					directional_light.draw_debug_lines(cmd, current_frame, gui_input.nr_shadow_cascades);
				

				line_render_pass.end(cmd);
				cmd.end_debug_zone();
			}
			
			/*
			// transitions for copy into swapchain images
			Texture::transition_layout(cmd, color_resolve_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// copy (both have swapchain extent resolution
			Texture::copy(cmd, color_resolve_target, swapchain.images_at(current_swapchain_image_idx), swapchain.get_extent(), swapchain.get_extent());

			// transition for imgui
			Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			*/

			Texture::transition_layout(cmd, color_resolve_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// THIS IS CURSED
			{
				tone_mapper.begin(cmd,
					swapchain.get_extent(),
					swapchain.image_views_at(current_swapchain_image_idx),
					VK_NULL_HANDLE, // No depth buffer
					VK_NULL_HANDLE, // No color resolve
					VK_NULL_HANDLE, // No depth resolve
					true
				);

				view_descriptor_sets[current_frame].bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, tone_mapper.get_pipeline_layout(), 0);
				
				tone_mapper.material.bind(cmd, current_frame, { tone_mapper.view_plane.get_vertex_address(), gui_input.tone_mapper_mode, gui_input.luminance_white_point });
				tone_mapper.view_plane.draw(cmd, current_frame);

				tone_mapper.end(cmd);
			}


			{
				TracyVkZone(get_current_tracy_context(), cmd, "Imgui");
				cmd.begin_debug_zone("ImGUI Pass");

				// draw imgui
				gui.draw(cmd, current_swapchain_image_idx);

				cmd.end_debug_zone();
			}
			
			// transition for present
			Texture::transition_layout(cmd, swapchain.images_at(current_swapchain_image_idx), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
	
		TracyVkCollect(get_current_tracy_context(), cmd);

		// end and submit command buffer

		cmd.submit(
			{ get_current_swapchain_semaphore(), directional_light.get_shadow_pass_semaphore(current_frame) },
			{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },
			{ get_current_render_semaphore() },
			get_current_render_fence()
		);
	}
	
}

void Engine::present()
{
	ZoneScoped;

	if (!swapchain.present({ get_current_render_semaphore() }, current_swapchain_image_idx)) {
		spdlog::warn("Recreate swapchain (Present)");
		resize_window = true;
	}
}

void Engine::late_update()
{
	ZoneScoped;

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::init_logger()
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	std::string log_path = "logs/log.txt";
	std::remove(log_path.c_str());
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path);

#ifdef NDEBUG
	console_sink->set_level(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::make_shared<spdlog::logger>("Logger", spdlog::sinks_init_list({ console_sink, file_sink })));
}

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

	init_descriptor_set_layouts();
	create_descriptor_set_pool();

	create_render_passes();

	init_data();
	init_descriptor_sets();
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

	spdlog::info("Selected GPU: {}", device.get_device_properties().deviceName);
	spdlog::info("Tesselation limit: {}", device.get_device_properties().limits.maxTessellationGenerationLevel);
}

void Engine::create_queues()
{
	graphics_queue.init(device, vkb::QueueType::graphics, "Graphics queue");
	present_queue.init(device, vkb::QueueType::present, "Present queue");
	transfer_queue.init(device, vkb::QueueType::transfer, "Transfer queue");
	spdlog::info("Chosen queues; Graphics: {}, Present: {}, Transfer: {}", graphics_queue.get_queue_family(), present_queue.get_queue_family(), transfer_queue.get_queue_family());
}

void Engine::init_descriptor_set_layouts()
{
	view_desc_set_layout.add_binding(
		0, 
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
		VK_SHADER_STAGE_ALL
	);
	view_desc_set_layout.init(&device, "Shared view resource set layout");
	cleanup_queue.add(&view_desc_set_layout);

	shadow_desc_set_layout = DirectionalLight::create_shadow_descriptor_layout(device);
	cleanup_queue.add(&shadow_desc_set_layout);

	terrain_desc_set_layout = Terrain::create_descriptor_set_layout(device);
	cleanup_queue.add(&terrain_desc_set_layout);

	environment_desc_set_layout = EnvironmentMap::create_descriptor_set_layout(device);
	cleanup_queue.add(&environment_desc_set_layout);

	pbr_desc_set_layout = ObjMesh::create_descriptor_set_layout(device);
	cleanup_queue.add(&pbr_desc_set_layout);

	cpu_text_sample_set_layout = Texture::create_cpu_sample_descriptor_set_layout(device);
	cleanup_queue.add(&cpu_text_sample_set_layout);

	tone_mapper_desc_set_layout = ToneMapper::create_descriptor_set_layout(device);
	cleanup_queue.add(&tone_mapper_desc_set_layout);
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
		50,                 // distance from look at for projection
		1024 * 4,             // resolution
		1024 * 4,
		40,                 // height of orthographic projection
		0.1f,                // near
		50.0f                // far plane
	);

	// can toggle debug drawings of cascade frustums
	directional_light.init_debug_lines(
		get_current_transfer_pool(),
		descriptor_pool,
		line_render_pass
	);

	cleanup_queue.add(&directional_light);

	// terrain needs directional light to be initiated due to it relying on it's uniform buffer
	// try manticorp.github.io/unrealheightmap
	terrain.init(
		device,
		get_current_graphics_pool(),
		get_current_transfer_pool(),
		descriptor_pool,
		&mirror_texture_sampler,
		terrain_render_passes[2],

		"textures/terrain/heightmap.png", // height map
		"textures/terrain/texture.png",   // albedo
		"textures/terrain/normal.png",	  // normals TODO: support normal computation directly from heightmap
		256							      // resolution of base mesh
	);
	terrain.set_descriptor_bindings();
	cleanup_queue.add(&terrain);

	environment_map.init(
		device,
		get_current_graphics_pool(),
		get_current_transfer_pool(),
		descriptor_pool,
		environment_render_pass,

		"textures/environment_maps/day_cube_map_%.exr"
	);
	environment_map.set_descriptor_bindings(linear_texture_sampler);
	cleanup_queue.add(&environment_map);

	texture_not_found = create_texture_from_path(
		&device,
		&get_current_graphics_pool(),
		"textures/texture_not_found.png",
		Texture_Type::Tex_RGB,
		"Texture Not Found fallback"
	);
	cleanup_queue.add(&texture_not_found);

	{
		
		meshes[0].init(device, get_current_graphics_pool(), get_current_transfer_pool(), descriptor_pool, pbr_render_pass, "models/baloon.obj");
		meshes[0].set_descriptor_bindings(texture_not_found, linear_texture_sampler);
		cleanup_queue.add(&meshes[0]);

		/*
		meshes[1].init(device, get_current_graphics_pool(), get_current_transfer_pool(), descriptor_pool, pbr_render_pass, "models/plane.obj");
		meshes[1].set_descriptor_bindings(texture_not_found, linear_texture_sampler);
		cleanup_queue.add(&meshes[1]);

		meshes[2].init(device, get_current_graphics_pool(), get_current_transfer_pool(), descriptor_pool, pbr_render_pass, "models/material_tests/mitsuba_texture.obj");
		meshes[2].set_descriptor_bindings(texture_not_found, linear_texture_sampler);
		cleanup_queue.add(&meshes[2]);

		meshes[3].init(device, get_current_graphics_pool(), get_current_transfer_pool(), dyn_descriptor_pool, pbr_render_pass, "models/trees/Tree0.obj");
		//meshes[3].init(device, get_current_graphics_pool(), get_current_transfer_pool(), dyn_descriptor_pool, pbr_render_pass, "models/sponza/sponza.obj");
		meshes[3].set_descriptor_bindings(texture_not_found, linear_texture_sampler);
		cleanup_queue.add(&meshes[3]);
		*/
	}

	{
		// procedural tree placement (height cut off and not on green)
		std::default_random_engine generator{};
		std::uniform_real_distribution<float> distribution{ 0, 1};

		const int nr_instances = 1024*3;

		std::vector<glm::vec2> uv_samples{};
		uv_samples.reserve(2*nr_instances);
		for (int i = 0; i < 2*nr_instances; i++) {
			uv_samples.push_back({ distribution(generator), distribution(generator) });
		}

		std::vector<glm::vec4> height_res{};
		terrain.get_height_map().cpu_texture_samples(get_current_graphics_pool(), descriptor_pool, cpu_text_sample_set_layout, linear_texture_sampler, uv_samples, height_res);

		std::vector<glm::vec4> albedo_res{};
		terrain.get_albedo().cpu_texture_samples(get_current_graphics_pool(), descriptor_pool, cpu_text_sample_set_layout, linear_texture_sampler, uv_samples, albedo_res);

		std::vector<InstanceData> per_instance_data{};
		per_instance_data.reserve(nr_instances);

		// this might not create nr_instances many instances
		for (int i = 0; i < 2*nr_instances; i++) {
			bool height_cutoff = height_res[i].x < 0.6f;
			bool non_green = glm::dot(albedo_res[i], { 85.f/255, 99.f / 255, 60.f / 255, 1}) > 1.125f;
			if (per_instance_data.size() < nr_instances && height_cutoff && non_green) {
				per_instance_data.push_back({{
					(uv_samples[i].x - 0.5) * 2 * 25,
					(uv_samples[i].y - 0.5) * 2 * 25,
					height_res[i].x * 25 * 0.8
				}});
			}
		}

		std::vector <InstancedShape<ObjMesh>> meshes{};
		std::vector<VKW_Path> mesh_path{ "models/trees/Tree0.obj", "models/trees/Tree1.obj", "models/trees/Tree2.obj", "models/trees/Tree3.obj" };

		for (const VKW_Path& path : mesh_path) {
			ObjMesh mesh{};
			mesh.init(
				device, get_current_graphics_pool(), get_current_transfer_pool(), descriptor_pool, pbr_render_pass,
				path
			);
			mesh.set_descriptor_bindings(texture_not_found, linear_texture_sampler);
			
			InstancedShape<ObjMesh> instanced_mesh{};
			instanced_mesh.init(device, get_current_transfer_pool(),
				std::move(mesh),
				nr_instances,
				{},
				true
			);
			
			meshes.push_back(instanced_mesh);
		}

		lod_mesh.init(
			std::move(meshes),
			per_instance_data
		);
		cleanup_queue.add(&lod_mesh);
	}

	tone_mapper.init(
		device, 
		get_current_transfer_pool(),
		descriptor_pool,
		{view_desc_set_layout, tone_mapper_desc_set_layout}, // TODO tone mapper desc set layout
		swapchain.get_format() // will write to swapchain
	);

	// needs to also be called whenever we recreate our images due to resize
	tone_mapper.set_descriptor_bindings(
		{ color_resolve_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), color_resolve_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT) },
		linear_texture_sampler
	);
	cleanup_queue.add(&tone_mapper);
}

void Engine::init_descriptor_sets()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VKW_DescriptorSet& view_desc_set = view_descriptor_sets[i];
		view_desc_set.init(
			&device,
			&descriptor_pool,
			view_desc_set_layout,
			fmt::format("View Desc Set ({})", i)
		);
		view_desc_set.update(0, uniform_buffers.at(i));
		cleanup_queue.add(&view_desc_set);

		VKW_DescriptorSet& shadow_desc_set = shadow_descriptor_sets[i];
		shadow_desc_set.init(
			&device,
			&descriptor_pool,
			shadow_desc_set_layout,
			fmt::format("Shadow Desc Set ({})", i)
		);

		shadow_desc_set.update(0, directional_light.get_uniform_buffers().at(i));
		shadow_desc_set.update(1, directional_light.get_texture().get_image_view(VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, MAX_CASCADE_COUNT));
		shadow_desc_set.update(2, linear_texture_sampler);
		shadow_desc_set.update(3, shadow_map_gather_sampler);
		cleanup_queue.add(&shadow_desc_set);
	}
}

void Engine::create_texture_samplers()
{
	linear_texture_sampler.init(&device, "Default sampler");
	cleanup_queue.add(&linear_texture_sampler);

	nearest_texture_sampler.set_min_filter(VK_FILTER_NEAREST);
	nearest_texture_sampler.set_mag_filter(VK_FILTER_NEAREST);
	nearest_texture_sampler.init(&device, "Nearest neighbour sampler");
	cleanup_queue.add(&nearest_texture_sampler);

	mirror_texture_sampler.set_address_mode(VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
	mirror_texture_sampler.init(&device, "Repeat sampler");
	cleanup_queue.add(&mirror_texture_sampler);

	shadow_map_gather_sampler.set_comparison(VK_COMPARE_OP_LESS);
	shadow_map_gather_sampler.init(&device, "Shadow map gather sampler");
	cleanup_queue.add(&shadow_map_gather_sampler);
}

void Engine::create_descriptor_set_pool()
{
	imgui_descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
	imgui_descriptor_pool.init(&device, 1, "Imgui descriptor pool");
	cleanup_queue.add(&imgui_descriptor_pool);
	
	// TODO Better system
	descriptor_pool.add_layout(view_desc_set_layout, MAX_FRAMES_IN_FLIGHT*(2*MAX_CASCADE_COUNT + 2 + 4*4));
	descriptor_pool.add_layout(shadow_desc_set_layout, 4*4+ 1*MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_layout(terrain_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_layout(environment_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_layout(pbr_desc_set_layout, 4 * 4 * MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_layout(cpu_text_sample_set_layout, 1);
	descriptor_pool.add_layout(tone_mapper_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1); // precompute curvature
	descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1); // precompute curvature

	descriptor_pool.init(&device, MAX_FRAMES_IN_FLIGHT*(5 + 12*4 + 2 * MAX_CASCADE_COUNT) + 1, "General descriptor pool");
	cleanup_queue.add(&descriptor_pool);

	dyn_descriptor_pool.add_layout(view_desc_set_layout, MAX_FRAMES_IN_FLIGHT );
	dyn_descriptor_pool.add_layout(shadow_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	dyn_descriptor_pool.add_layout(terrain_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	dyn_descriptor_pool.add_layout(environment_desc_set_layout, MAX_FRAMES_IN_FLIGHT);
	dyn_descriptor_pool.add_layout(pbr_desc_set_layout, 4 * 4 * MAX_FRAMES_IN_FLIGHT);
	dyn_descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1); // precompute curvature
	dyn_descriptor_pool.add_type(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1); // precompute curvature

	dyn_descriptor_pool.init(&device, MAX_FRAMES_IN_FLIGHT * (21), "General dynamic descriptor pool");
	cleanup_queue.add(&dyn_descriptor_pool);
}

void Engine::create_uniform_buffers()
{
	for (VKW_Buffer& uniform_buffer : uniform_buffers) {
		uniform_buffer.init(
			&device, 
			sizeof(UniformStruct), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			sharing_exlusive(), 
			Mapping::Persistent,
			"Main Uniform buffer"
		);
		cleanup_queue.add(&uniform_buffer);
	}
}

void Engine::init_render_targets()
{
	// Scene rendered into this texture before copied over, allows for higher resolution than what is used in the swapchain
	// This image is in linear color space (no conversion durng texture read/writes)
	// The blit/copy over into the swapchain image does the conversion (due to it being sRGB)

	color_render_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_Colortarget),
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		sharing_exlusive(),
		"Color render target",
		1, // mip layers
		sample_count
	); // TODO: FIX might add it multiple times to the clean up queue	
	cleanup_queue.add(&color_render_target);
	
	color_resolve_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_Colortarget),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		sharing_exlusive(),
		"Color resolve target"
	); // TODO: FIX might add it multiple times to the clean up queue	
	cleanup_queue.add(&color_resolve_target);

	depth_render_target.init(
		&device,
		swapchain.get_extent().width,
		swapchain.get_extent().height,
		Texture::find_format(device, Texture_Type::Tex_D),
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		sharing_exlusive(),
		"Depth render target",
		1, // mip layers
		sample_count
	);
	cleanup_queue.add(&depth_render_target);
}

void Engine::create_render_passes()
{
	// have different specialized render passes for different cascade counts (specialization constants)
	std::array<VKW_DescriptorSetLayout, 3> descriptor_set_layouts{ view_desc_set_layout, shadow_desc_set_layout, terrain_desc_set_layout };
	
	for (int i = 0; i < MAX_CASCADE_COUNT; i++) {
		terrain_render_passes.at(i) = Terrain::create_render_pass(
			&device,
			descriptor_set_layouts,
			color_render_target,
			depth_render_target,
			sample_count,
			false, // not depth only
			false, // not wireframe
			false, // not bias depth
			i + 1
		);
		cleanup_queue.add(&terrain_render_passes.at(i));

		terrain_wireframe_render_passes.at(i) = Terrain::create_render_pass(
			&device,
			descriptor_set_layouts, 
			color_render_target,
			depth_render_target,
			sample_count,
			false, // not depth only
			true,  // wireframe
			false, // not bias depth
			i + 1
		);
		cleanup_queue.add(&terrain_wireframe_render_passes.at(i));
	}

	terrain_depth_render_pass = Terrain::create_render_pass(
		&device,
		descriptor_set_layouts, 
		color_render_target,
		depth_render_target,
		VK_SAMPLE_COUNT_1_BIT,
		true,  // depth only
		false, // not wireframe
		true   // bias depth
	);
	cleanup_queue.add(&terrain_depth_render_pass);

	environment_render_pass = EnvironmentMap::create_render_pass(&device, { view_desc_set_layout, environment_desc_set_layout}, color_render_target, depth_render_target, sample_count);
	cleanup_queue.add(&environment_render_pass);

	line_render_pass = Line::create_render_pass(&device, { view_desc_set_layout }, color_render_target, depth_render_target, sample_count);
	cleanup_queue.add(&line_render_pass);

	pbr_render_pass = ObjMesh::create_render_pass(&device, { view_desc_set_layout, shadow_desc_set_layout, pbr_desc_set_layout }, color_render_target, depth_render_target, sample_count);
	cleanup_queue.add(&pbr_render_pass);

	pbr_render_double_sided_pass = ObjMesh::create_render_pass(&device, { view_desc_set_layout, shadow_desc_set_layout, pbr_desc_set_layout }, color_render_target, depth_render_target, sample_count, false, false, false);
	cleanup_queue.add(&pbr_render_double_sided_pass);

	pbr_depth_pass = ObjMesh::create_render_pass(&device, { view_desc_set_layout, shadow_desc_set_layout, pbr_desc_set_layout }, color_render_target, depth_render_target, VK_SAMPLE_COUNT_1_BIT, true, true);
	cleanup_queue.add(&pbr_depth_pass);
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

	// needs to be called whenever we recreate our images due to resize
	tone_mapper.set_descriptor_bindings(
		{ color_resolve_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), color_resolve_target.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT) },
		linear_texture_sampler
	);

	camera.set_aspect_ratio(res_x, res_y);
}

void Engine::recreate_render_targets()
{
	// Destroys render targets
	color_render_target.del();
	depth_render_target.del();
	color_resolve_target.del();
	
	// important to clear i.e. image view's cache
	color_render_target = {};
	depth_render_target = {};
	color_resolve_target = {};
	
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
			{},
		};

		command_structs.at(i).graphics_command_pool.init(&device, &graphics_queue, "Graphics pool");
		cleanup_queue.add(&command_structs.at(i).graphics_command_pool);
		
		// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: short lived command buffers
		command_structs.at(i).transfer_command_pool.init(&device, &transfer_queue, "Transfer pool");
		cleanup_queue.add(&command_structs.at(i).transfer_command_pool);
		
		command_structs.at(i).graphics_command_buffer.init(&device, &command_structs.at(i).graphics_command_pool, false, "Draw CMD");

		command_structs.at(i).graphics_queue_tracy_context = TracyVkContextCalibrated(device.get_physical_device(), device, graphics_queue, command_structs.at(i).graphics_command_buffer, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
		TracyVkContextName(command_structs.at(i).graphics_queue_tracy_context, "Graphics Context", sizeof("Graphics Context"));
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

		device.name_object((uint64_t)sync_structs.at(i).swapchain_semaphore, VK_OBJECT_TYPE_SEMAPHORE, fmt::format("Swapchain semaphore {}", i) );
		device.name_object((uint64_t)sync_structs.at(i).render_semaphore, VK_OBJECT_TYPE_SEMAPHORE, fmt::format("Render semaphore {}", i));
		device.name_object((uint64_t)sync_structs.at(i).render_fence, VK_OBJECT_TYPE_FENCE, fmt::format("Render fence{}", i));
	}
}

bool Engine::aquire_image()
{
	ZoneScoped;

	// Wait such that we are not still drawing into this frame (but we might not have presented it properly)
	VkFence render_fence = get_current_render_fence();
	VK_CHECK_E(vkWaitForFences(device, 1, &render_fence, VK_TRUE, UINT64_MAX), RuntimeException);
	
	VkResult aquire_image_result = vkAcquireNextImageKHR(
		device, 
		swapchain,
		UINT64_MAX, 
		get_current_swapchain_semaphore(), // signal swapchain semaphore once we aquired image
		VK_NULL_HANDLE, 
		&current_swapchain_image_idx
	);
	if (aquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
		spdlog::warn("Recreate swapchain (Aquire)");
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
	ZoneScoped;

	glfw_input_mutex.lock();

	UniformStruct uniform{};
	uniform.proj = camera.generate_projection_mat();
	uniform.proj[1][1] *= -1; // see https://community.khronos.org/t/confused-when-using-glm-for-projection/108548/2 for reason for the multiplication

	uniform.view = camera.generate_view_mat();
	uniform.inv_view = glm::inverse(uniform.view);
	uniform.virtual_view = camera.generate_virtual_view_mat();

	uniform.near_far_plane = glm::vec2(camera.get_near_plane(), camera.get_far_plane());
	glfw_input_mutex.unlock();

	uniform_buffers.at(current_frame).copy_into(&uniform, sizeof(UniformStruct));
}

std::vector<const char*> Engine::get_required_instance_extensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
	extensions.push_back("VK_EXT_debug_utils");
#endif

	return extensions;
}

std::vector<const char*> Engine::get_required_device_extensions()
{
	// VK_KHR_SWAPCHAIN_EXTENSION_NAME is added automatically
	std::vector<const char*> extensions{};

#ifdef TRACY_ENABLE
	extensions.push_back("VK_EXT_calibrated_timestamps");
#endif

	return extensions;
}

Required_Device_Features Engine::get_required_device_features()
{
	Required_Device_Features features{};
	// 1.0 features
	features.rf.samplerAnisotropy = true;
	features.rf.fillModeNonSolid = true;
	features.rf.tessellationShader = true;
	features.rf.shaderInt64 = true;
	// 1.2 features
	features.rf12.bufferDeviceAddress = true;
	features.rf12.descriptorIndexing = true;
	features.rf12.uniformBufferStandardLayout = true; // enable std430 for uniform buffers
	// 1.3 features
	features.rf13.dynamicRendering = true;
	features.rf13.synchronization2 = true;
	features.rf13.shaderDemoteToHelperInvocation = true; // for discard in some shaders

	return features;
}

void glfm_mouse_move_callback(GLFWwindow* window, double pos_x, double pos_y) {
	Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (engine) {
		ZoneScopedN("Mouse callback");

		engine->get_glfw_input_recursive_mutex().lock();
		engine->get_camera_controller().handle_mouse(pos_x, pos_y);
		engine->get_glfw_input_recursive_mutex().unlock();
	}
	else {
		// TODO THIS MIGHT BE UNDEFINED BEHAVIOR
		throw SetupException("GLFW Engine User pointer not set", __FILE__, __LINE__);
	}
}