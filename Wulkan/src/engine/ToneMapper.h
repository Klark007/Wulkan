#pragma once
#include "Renderpass.h"
#include "Mesh.h"

struct ToneMapperPushConstants {
	alignas(8) VkDeviceAddress vertex_buffer;
};
constexpr size_t TONE_MAPPER_DESC_SET_COUNT = 2;

class ToneMapper : public RenderPass< ToneMapperPushConstants, TONE_MAPPER_DESC_SET_COUNT>
{
public:
	ToneMapper() = default;

	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, const std::array<VKW_DescriptorSetLayout, 2>& layouts, VkFormat color_attachment_format);
	void set_descriptor_bindings(const std::array<VkImageView, MAX_FRAMES_IN_FLIGHT>& views, const VKW_Sampler& texture_sampler);
	void del() override;

	static VKW_DescriptorSetLayout create_descriptor_set_layout(const VKW_Device& device);
	
	// tone maps from ... to ...
private:
	inline static VKW_DescriptorSetLayout descriptor_set_layout;
public: // TODO remove
	MaterialInstance< ToneMapperPushConstants, 1> material;
	Mesh view_plane; // plane spanning full view
};

