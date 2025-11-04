#pragma once

#include "Material.h"
#include "Path.h"
#include "Renderpass.h"
#include "Texture.h"
#include "Shape.h"

struct PBRUniform {
	alignas(16) glm::vec3 diffuse;
	alignas(4) float metallic;

	alignas(16) glm::vec3 specular;
	alignas(4) float roughness;

	alignas(16) glm::vec3 emission;
	alignas(4) float eta;
	alignas(4) uint32_t configuration; // 0 bit: if true, read from albedo texture; upper 16 bit for setting visualization mode
};

constexpr size_t PBR_MAT_DESC_SET_COUNT = 3;
class PBRMaterial : public MaterialInstance< PushConstants, 1>{
public:
	PBRMaterial() = default;

	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, VKW_DescriptorPool& descriptor_pool,  RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const std::array<VKW_DescriptorSetLayout, 1>& descriptor_layouts, const std::array<uint32_t, 1>& set_slots, const PBRUniform& uniform, const VKW_Path& parent_path, const VKW_Path& diffuse_path, const std::string& material_name);

	void del() override;
private:
	PBRUniform m_uniform;
	VisualizationMode m_visualization_mode;

	std::optional<Texture> m_diffuse_texture;
	std::array< VKW_Buffer, MAX_FRAMES_IN_FLIGHT> m_uniform_buffers;
public:
	inline const VKW_Buffer& get_uniform_buffer(uint32_t current_frame) const { return m_uniform_buffers[current_frame]; };
	inline Texture& get_diffuse_texture(Texture& fallback);
	inline void set_visualization_mode(VisualizationMode mode);
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

inline void PBRMaterial::set_visualization_mode(VisualizationMode mode)
{
	if (mode != m_visualization_mode) {
		//spdlog::info("Old uniform config {:032b}", m_uniform.configuration);
		// keep lower 16 bits as pervious
		uint32_t conifg_kept = m_uniform.configuration & ((1 << 16) - 1);
		m_uniform.configuration = conifg_kept | (static_cast<uint32_t>(mode) << 16);
		
		for (size_t frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; frame_idx++) {
			VKW_Buffer& uniform_buffer = m_uniform_buffers[frame_idx];
			memcpy(uniform_buffer.get_mapped_address(), &m_uniform, sizeof(PBRUniform));
		}
		
		m_visualization_mode = mode;
	}
}
