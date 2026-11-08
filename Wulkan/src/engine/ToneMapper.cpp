#include "common.h"
#include "ToneMapper.h"
#include "Gui.h"


void ToneMapper::init(const VKW_Device* device, const VKW_CommandPool* graphics_pool, const VKW_CommandPool* transfer_pool, VKW_DescriptorPool& descriptor_pool, const std::array<VKW_DescriptorSetLayout, 2>& layouts, std::span<VkFormat> color_attachment_formats, unsigned int bake_resolution)
{
	m_device = device;
	m_transfer_pool = transfer_pool;
	m_graphics_pool = graphics_pool;
	m_graphics_queue_family_idx = graphics_pool->get_queue()->get_queue_family();
	m_transfer_queue_family_idx = transfer_pool->get_queue()->get_queue_family();

	// hard coded view plane
	const std::vector<Vertex> vertices = {
		{glm::vec3(0.0,0.0,0.0), 0.0, {}, 0.0, {}},
		{glm::vec3(0.0,1.0,0.0), 0.0, {}, 1.0, {}},
		{glm::vec3(1.0,0.0,0.0), 1.0, {}, 0.0, {}},
		{glm::vec3(1.0,1.0,0.0), 1.0, {}, 1.0, {}}
	};
	const std::vector<uint32_t> indices = { 0,1,2,1,3,2 };

	view_plane.init(*device, *transfer_pool, vertices, indices);

	// push constants
	VKW_PushConstant<ToneMapperPushConstants> push_constant{};
	push_constant.init(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	// graphics pipeline	
	// defaults: no depth test / write, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	VKW_GraphicsPipeline graphics_pipeline{};

	VKW_Shader vert_shader{};
	vert_shader.init(device, "shaders/post_processing/tonemap_slang.spv", VK_SHADER_STAGE_VERTEX_BIT, "ToneMapper vertex shader", "vertexMain");


	VKW_Shader frag_shader{};
	frag_shader.init(device, "shaders/post_processing/tonemap_slang.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "ToneMapper fragment shader", "fragmentMain");

	graphics_pipeline.add_shader_stages({ vert_shader, frag_shader });
	
	graphics_pipeline.add_descriptor_sets(layouts);
	graphics_pipeline.add_push_constants({ push_constant.get_range() });

	graphics_pipeline.set_color_attachment_format(color_attachment_formats);

	graphics_pipeline.init(device, "Tone Mapping graphics pipeline");

	vert_shader.del();
	frag_shader.del();

	m_resolution = bake_resolution;
	VkFormat baked_texture_format = Texture::find_format(*device, Texture_Type::Tex_R_Linear);
	

	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		graphics_pool,
		true,
		"Curve Transition CMD"
	);
	command_buffer.begin_single_use();


	for (int i = 0; i < 2; i++) {
		m_baked_curves[i].init(
			device,
			m_resolution,
			1,
			baked_texture_format,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ VkSharingMode::VK_SHARING_MODE_CONCURRENT, { m_graphics_queue_family_idx, m_transfer_queue_family_idx} }, // TODO: make exclusive with explicit handling of ownership transfers
			fmt::format("Baked tone curve {}", i)
		);

		Texture::transition_layout(
			command_buffer,
			m_baked_curves[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
	command_buffer.submit_single_use();

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VK_CHECK_E(vkCreateFence(*device, &fence_info, nullptr, &m_staging_fence), SetupException);;

	VkDeviceSize staging_buffer_size = m_resolution * sizeof(unsigned char);
	m_staging_buffer.init(
		device,
		staging_buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		sharing_exlusive(),
		Mapping::Mapped,
		"Baked tone curve staging buffer"
	);
	m_staging_buffer_data.resize(m_resolution);

	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (int i = 0; i < 2; i++) {
		VK_CHECK_E(vkCreateSemaphore(*device, &semaphore_create_info, nullptr, &m_release_semaphores.at(i)), SetupException);
	}
	

	// super init
	RenderPass::init(std::move(graphics_pipeline), layouts, push_constant);

	material.init(*device, descriptor_pool, *this, { descriptor_set_layout }, { 1 }, "Tone Mapper Material");
}

void ToneMapper::set_descriptor_bindings(const std::array<VkImageView, MAX_FRAMES_IN_FLIGHT>& views, const VKW_Sampler& frame_sampler, const VKW_Sampler& baked_curve_sampler)
{
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		const VKW_DescriptorSet& set0 = material.get_descriptor_set(i, 0);
		set0.update(0, views[i], frame_sampler);
		set0.update(1, m_baked_curves[m_current_render_curve].get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), baked_curve_sampler);
	}
}

void ToneMapper::del()
{
	material.del();
	RenderPass::del();
	view_plane.del();

	for (Texture& curve: m_baked_curves) {
		curve.del();
	}
	VK_DESTROY(m_staging_fence, vkDestroyFence, *m_device, m_staging_fence);
	m_staging_buffer.del();

	for (VkSemaphore& semaphore : m_release_semaphores) {
		VK_DESTROY(semaphore, vkDestroySemaphore, *m_device, semaphore);
	}
}

void ToneMapper::do_pass(const VKW_CommandBuffer& cmd, uint32_t current_frame, VkImageView swapchain_img_view, VkExtent2D swapchain_extend, VkImageView screenshot_img_view, const VKW_DescriptorSet& view_descriptor_set)
{
	if (m_graphics_aquire_ownership) {
		// aquire ownership from transfer queue
		Texture::transition_layout(
			cmd,
			m_baked_curves[m_current_render_curve],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			m_transfer_queue_family_idx,
			m_graphics_queue_family_idx
		);

		m_graphics_aquire_ownership = false;
	}


	std::array<VkImageView, 2> tone_mapper_rts{ swapchain_img_view, screenshot_img_view };
	begin(cmd,
		swapchain_extend,
		tone_mapper_rts,
		VK_NULL_HANDLE, // No depth buffer
		VK_NULL_HANDLE, // No color resolve
		VK_NULL_HANDLE, // No depth resolve
		true
	);

	view_descriptor_set.bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, get_pipeline_layout(), 0);

	material.bind(cmd, current_frame, { 
		.vertex_buffer = view_plane.get_vertex_address(), 
		.mode = m_input->tone_mapper_mode, 
		.luminance_white_point = (m_input->tone_mapper_mode == ToneMapperMode::PowerCurves) ? m_power_w : m_input->luminance_white_point 
		}
	);
	view_plane.draw(cmd, current_frame);

	end(cmd);
}

void ToneMapper::update(const VKW_Device& device, const VKW_Sampler& baked_curve_sampler, GUI_Input* input, unsigned int current_frame)
{
	m_input = input;
	if (m_input->rebake_power_curves) {
		m_input->rebake_power_curves = false;

		bake_filmic_power(m_input->power_curve_params);
	}

	if (m_set_desc_set) {
		// change happened last frame, also update desc set of this frame
		m_set_desc_set = false;
		material.get_descriptor_set(current_frame, 0).update(1, m_baked_curves[m_current_render_curve].get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), baked_curve_sampler);
	}

	if (m_upload_in_flight) {
		// check if finished
		VkResult fence_status = vkGetFenceStatus(device, m_staging_fence);
		if (fence_status == VK_SUCCESS) {
			vkResetFences(device, 1, &m_staging_fence);

			// delete cmd buffer
			m_transfer_cmd_buffer.del();
			m_graphics_cmd_buffer.del();

			m_upload_in_flight = false;

			m_current_render_curve = (m_current_render_curve + 1) % 2;

			m_set_desc_set = true; // update for other frame as well
			const VKW_DescriptorSet& set0 = material.get_descriptor_set(current_frame, 0);
			set0.update(1, m_baked_curves[m_current_render_curve].get_image_view(VK_IMAGE_ASPECT_COLOR_BIT), baked_curve_sampler);

			// change w in push constant
			m_power_w = m_uploading_W;

			m_graphics_aquire_ownership = true; // will need to get ownership in the graphics queue
		}
		else if (fence_status == VK_ERROR_DEVICE_LOST) {
			throw RuntimeException("Device lost", __FILE__, __LINE__);
		}
	}
	else if (m_do_upload) {
		// start an upload
		upload(device);
	}
}

VKW_DescriptorSetLayout ToneMapper::create_descriptor_set_layout(const VKW_Device& device)
{
	descriptor_set_layout = VKW_DescriptorSetLayout{};

	descriptor_set_layout.add_binding(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);
	
	descriptor_set_layout.add_binding(
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT
	);

	descriptor_set_layout.init(&device, "Tone Mapper Desc Layout");

	return descriptor_set_layout;
}

float ToneMapper::evaluate_filmic_power(FilmicPowerCurveSegment& curve, float x)
{
	float x0 = (x - curve.m_offset_x) * curve.m_scale_x;
	if (x0 == 0) {
		return curve.m_offset_y;
	}

	return curve.m_scale_y * exp(curve.m_ln_a + curve.m_b * log(x0)) + curve.m_offset_y;
}

void ToneMapper::bake_filmic_power(FilmicPowerUserParams user_params)
{
	m_do_upload = true;

	// compute x0, x1, w etc.
	// toe section
	float x0 = user_params.m_toe_length * 0.5f;
	float y0 = (1 - user_params.m_toe_strength) * x0; // lerp from 0 to x0

	float remaining_y = 1 - y0;

	// how big is our linear section
	float y_offset = (1 - user_params.m_shoulder_length) * remaining_y;
	float x1 = x0 + y_offset;
	float y1 = y0 + y_offset;

	// filmic shoulder strength is in F stops
	float extraW = exp(user_params.m_shoulder_strength) - 1.0f;
	float W = x0 + remaining_y + extraW;

	m_staging_W = W; // cant be used immediately as push constant

	// compute sections power curves
	// first rescale x to be within [0,1]
	float W_inv = 1 / W;
	x0 *= W_inv;
	x1 *= W_inv;

	// first linear section 
	float m, b;
	{
		// function y = mx + b
		if (x1 - x0 == 0) {
			m = 1;
		} else {
			m = (y1 - y0) / (x1 - x0);
		}
		b = y0 - x0 * m;

		// rewrite exp(ln(y)) = y = exp (ln(m) + ln(x + b/m))
		FilmicPowerCurveSegment linear{};
		linear.m_ln_a = log(m);
		linear.m_b = 1;
		linear.m_offset_x = -(b / m);
		linear.m_offset_y = 0;
		linear.m_scale_x = 1;
		linear.m_scale_y = 1;

		m_power_curves[1] = linear;
	}

	// toe section
	{
		FilmicPowerCurveSegment toe{};
		toe.m_offset_x = 0;
		toe.m_offset_y = 0;
		toe.m_scale_x = 1;
		toe.m_scale_y = 1;

		// compute a and b given X0 and Y0 and derivative at their point M (needs to be smooth)
		solve_lnA_B(toe.m_ln_a, toe.m_b, x0, y0, m);
		m_power_curves[0] = toe;
	}

	// shoulder
	{
		FilmicPowerCurveSegment shoulder{};
		shoulder.m_offset_x = 1;
		shoulder.m_offset_y = 1;
		shoulder.m_scale_x = -1;
		shoulder.m_scale_y = -1;

		// apply offset/scale
		float x = shoulder.m_scale_x * (x1 - shoulder.m_offset_x);
		float y = shoulder.m_scale_y * (y1 - shoulder.m_offset_y);
		solve_lnA_B(shoulder.m_ln_a, shoulder.m_b, x, y, m);
		m_power_curves[2] = shoulder;
	}

	for (int i = 0; i < m_resolution; i++) {
		// select segment
		float x = (float)i / m_resolution;
		int index = (x < x0) ? 0 : ((x < x1) ? 1 : 2);
		
		float y = evaluate_filmic_power(m_power_curves[index], x);

		m_staging_buffer_data[i] = static_cast<unsigned char>(round(y * 255));
	}
}

void ToneMapper::upload(const VKW_Device& device)
{
	unsigned int unused_texture = (m_current_render_curve + 1) % 2;

	m_do_upload = false;
	m_upload_in_flight = true;

	// update m_staging_buffer
	m_staging_buffer.copy_into(m_staging_buffer_data.data(), m_staging_buffer_data.size() * sizeof(unsigned char));
	m_uploading_W = m_staging_W;

	// release graphics ownership
	m_graphics_cmd_buffer.init(
		&device,
		m_graphics_pool,
		true,
		"Tonemap graphics release CMD"
	);

	m_graphics_cmd_buffer.begin_single_use();

	// release from graphics queue
	Texture::transition_layout(
		m_graphics_cmd_buffer,
		m_baked_curves[unused_texture],
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_graphics_queue_family_idx,
		m_transfer_queue_family_idx,
		0, // mip level
		VK_REMAINING_MIP_LEVELS,
		true // release ownership
	);

	m_graphics_cmd_buffer.submit({}, {}, {m_release_semaphores[unused_texture]}, VK_NULL_HANDLE);


	m_transfer_cmd_buffer.init(
		&device,
		m_transfer_pool,
		true,
		"Tonemap curve upload CMD"
	);

	m_transfer_cmd_buffer.begin_single_use();

	// explicit aquire from transfer queue
	Texture::transition_layout(
		m_transfer_cmd_buffer,
		m_baked_curves[unused_texture],
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_graphics_queue_family_idx,
		m_transfer_queue_family_idx
	);
	
	VkBufferImageCopy image_copy = create_buffer_image_copy((unsigned int)m_resolution, (unsigned int)1);
	vkCmdCopyBufferToImage(m_transfer_cmd_buffer, m_staging_buffer, m_baked_curves[unused_texture], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

	// explicit release from transfer queue
	Texture::transition_layout(
		m_transfer_cmd_buffer,
		m_baked_curves[unused_texture],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		m_transfer_queue_family_idx,
		m_graphics_queue_family_idx,
		0, // mip level
		VK_REMAINING_MIP_LEVELS,
		true // release ownership
	);

	m_transfer_cmd_buffer.submit({ m_release_semaphores[unused_texture] }, { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT }, {}, m_staging_fence);
}


// assumes scale and offset has been applied to the y values already
// f(x) = e^(ln(A) + B*ln((x)))
// have f(x) = y
// and f'(x) = m
void ToneMapper::solve_lnA_B(float& lnA, float& B, float x, float y, float m)
{
	B = (m * x) / y;
	lnA = log(y) - B * log(x);
}
