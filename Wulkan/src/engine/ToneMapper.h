#pragma once
#include "Renderpass.h"
#include "Mesh.h"

enum class ToneMapperMode {
	None,
	Rheinhard,
	ExtendedRheinhard,
	Uncharted,
	ACES,
	AgX,
	PowerCurves,
};

struct ToneMapperPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
	alignas(4) ToneMapperMode mode;
	alignas(4) float luminance_white_point;
};
constexpr size_t TONE_MAPPER_DESC_SET_COUNT = 2;

struct FilmicPowerCurveSegment {
	float m_offset_x;
	float m_offset_y;
	float m_scale_x;
	float m_scale_y;
	float m_ln_a;
	float m_b;
};

struct FilmicPowerUserParams {
	float m_toe_strength;      // as a ratio [0,1]
	float m_toe_length;        // as a ratio [0,1]
	float m_shoulder_strength; // in F stops (WRITTEN WRONG IN POST)
	float m_shoulder_length;   // as a ratio [0,1]
};

class ToneMapper : public RenderPass< ToneMapperPushConstants, TONE_MAPPER_DESC_SET_COUNT>
{
public:
	ToneMapper() = default;

	void init(const VKW_Device* device, const VKW_CommandPool* graphics_pool, const VKW_CommandPool* transfer_pool, VKW_DescriptorPool& descriptor_pool, const std::array<VKW_DescriptorSetLayout, 2>& layouts, std::span<VkFormat> color_attachment_format, unsigned int bake_resolution);
	void set_descriptor_bindings(const std::array<VkImageView, MAX_FRAMES_IN_FLIGHT>& views, const VKW_Sampler& frame_sampler, const VKW_Sampler& baked_curve_sampler);
	void del() override;

	void do_pass(const VKW_CommandBuffer& cmd, uint32_t current_frame, VkImageView swapchain_img_view, VkExtent2D swapchain_extend, VkImageView screenshot_img_view, const VKW_DescriptorSet& view_descriptor_set);
	void update(const VKW_Device& device, const VKW_Sampler& baked_curve_sampler, struct GUI_Input* input, unsigned int current_frame);

	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	float evaluate_filmic_power(FilmicPowerCurveSegment& curve, float x);
	void bake_filmic_power(FilmicPowerUserParams user_params);

private:
	void upload(const VKW_Device& device);

	const VKW_Device* m_device;
	const VKW_CommandPool* m_transfer_pool;
	const VKW_CommandPool* m_graphics_pool;
	uint32_t m_graphics_queue_family_idx;
	uint32_t m_transfer_queue_family_idx;

	VkFence m_staging_fence;
	VKW_Buffer m_staging_buffer;
	std::array<VkSemaphore, 2> m_release_semaphores;

	VKW_CommandBuffer m_graphics_cmd_buffer;
	VKW_CommandBuffer m_transfer_cmd_buffer;

	MaterialInstance< ToneMapperPushConstants, 1> material;
	Mesh view_plane; // plane spanning full view

	struct GUI_Input* m_input;

	std::array<FilmicPowerCurveSegment, 3> m_power_curves;

	std::array<Texture, 2> m_baked_curves;
	unsigned int m_resolution; // resolution of baked curve
	std::vector<unsigned char> m_staging_buffer_data;

	float m_power_w; // current W to be used
	float m_staging_W; // updated by every bake, what is currently in m_staging_buffer_data 
	float m_uploading_W; // updated every upload, what is currently in flight

	unsigned int m_current_render_curve = 0; // indicates which of the two curves is currently being used for rendering
	bool m_upload_in_flight = false; // true if currently are uploading, wait for fence to check if finished
	bool m_do_upload = false; // m_staging_buffer_data changed, need to upload again
	bool m_set_desc_set = false;
	bool m_graphics_aquire_ownership = false;

	inline static VKW_DescriptorSetLayout descriptor_set_layout;

	static void solve_lnA_B(float& lnA, float& B, float x, float y, float m);
};

