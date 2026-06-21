#pragma once
#include "Renderpass.h"
#include "Mesh.h"

enum class ToneMapperMode {
	None,
	Rheinhard,
	ExtendedRheinhard,
	Uncharted,
	ACES,
	AgX
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

	void init(const VKW_Device* device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, const std::array<VKW_DescriptorSetLayout, 2>& layouts, std::span<VkFormat> color_attachment_format, unsigned int bake_resolution);
	void set_descriptor_bindings(const std::array<VkImageView, MAX_FRAMES_IN_FLIGHT>& views, const VKW_Sampler& texture_sampler);
	void del() override;

	void update(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_CommandPool& graphics_pool);

	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);

	float evaluate_filmic_power(FilmicPowerCurveSegment& curve, float x);
	void bake_filmic_power(FilmicPowerUserParams user_params);

private:
	void upload(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const VKW_CommandPool& graphics_pool);

	const VKW_Device* m_device;

	std::array<FilmicPowerCurveSegment, 3> m_power_curves;
	std::array<Texture, 2> m_baked_curves;
	unsigned int m_current_render_curve = 0;; // indicates which of the two curves is currently being used for rendering
	unsigned int m_resolution;

	bool m_upload_in_flight = false; // true if currently are uploading, wait for fence to check if finished
	bool m_do_upload = false; // m_staging_buffer_data changed, need to upload again
	

	VkFence m_staging_fence;
	VKW_Buffer m_staging_buffer;
	VKW_CommandBuffer m_transfer_cmd_buffer;
	std::vector<unsigned char> m_staging_buffer_data;

	inline static VKW_DescriptorSetLayout descriptor_set_layout;

	static void solve_lnA_B(float& lnA, float& B, float x, float y, float m);
public: // TODO remove
	bool m_graphics_aquire_ownership = false; 
	MaterialInstance< ToneMapperPushConstants, 1> material;
	Mesh view_plane; // plane spanning full view
};

