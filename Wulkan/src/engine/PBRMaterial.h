#pragma once

#include "Material.h"
#include "Path.h"
#include "Renderpass.h"
#include "Texture.h"

struct PBRUniform {
	alignas(16) glm::vec3 diffuse;
	alignas(4) float metallic;

	alignas(16) glm::vec3 specular;
	alignas(4) float roughness;

	alignas(16) glm::vec3 emission;
	alignas(4) float eta;
	alignas(4) uint32_t configuration; // 0 bit: if true, read from albedo texture
};

constexpr size_t PBR_MAT_DESC_SET_COUNT = 3;
class PBRMaterial : public MaterialInstance< PushConstants, 1>{
public:
	PBRMaterial() = default;

	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_DescriptorPool& descriptor_pool,  RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const std::array<VKW_DescriptorSetLayout, 1>& descriptor_layouts, const std::array<uint32_t, 1>& set_slots, const PBRUniform& uniform, const VKW_Path& parent_path, const VKW_Path& diffuse_path, const std::string& material_name);

	void del() override;
private:
	std::optional<Texture> m_diffuse_texture;
	std::array< VKW_Buffer, MAX_FRAMES_IN_FLIGHT> m_uniform_buffers;
public:
	inline const VKW_Buffer& get_uniform_buffer(uint32_t current_frame) const { return m_uniform_buffers[current_frame]; };
	inline Texture& get_diffuse_texture(Texture& fallback);
};

Texture& PBRMaterial::get_diffuse_texture(Texture& fallback)
{
	if (m_diffuse_texture.has_value()) {
		return m_diffuse_texture.value();
	}
	else {
		return fallback;
	}
}