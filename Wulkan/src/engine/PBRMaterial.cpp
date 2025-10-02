#include "PBRMaterial.h"
#include "Path.h"

void PBRMaterial::init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const std::string& material_name, const VKW_CommandPool& graphics_pool, const PBRUniform& uniform, const std::string& diffuse_path)
{
	MaterialInstance< PushConstants, PBR_MAT_DESC_SET_COUNT>::init(
		device,
		descriptor_pool,
		render_pass,
		material_name
	);

	// create and set uniform buffers
	for (size_t frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; frame_idx++) {
		// TODO: different memory as we don't plan to change those on the fly
		VKW_Buffer& uniform_buffer = m_uniform_buffers[frame_idx];

		uniform_buffer.init(
			&device,
			sizeof(PBRUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sharing_exlusive(),
			true,
			fmt::format("PBR Uniform {} ({})", material_name, frame_idx)
		);
		uniform_buffer.map();

		memcpy(uniform_buffer.get_mapped_address(), &uniform, sizeof(PBRUniform));
	}

	if (diffuse_path != "") {
		VKW_Path diffuse_p { diffuse_path };
		if (!diffuse_p.is_absolute()) {
			// first check relative to material file
			diffuse_p = find_first_existing({
				"textures" / diffuse_p
			});
		}

		m_diffuse_texture = std::optional<Texture>{ create_texture_from_path(
				&device,
				&graphics_pool,
				diffuse_p,
				Texture_Type::Tex_RGB,
				fmt::format("{} diffuse texture", material_name)
			)
		};
	}
}

void PBRMaterial::del()
{
	MaterialInstance< PushConstants, PBR_MAT_DESC_SET_COUNT>::del();
	
	for (VKW_Buffer& uniform_buffer : m_uniform_buffers) {
		uniform_buffer.del();
	}

	if (m_diffuse_texture.has_value()) {
		m_diffuse_texture.value().del();
	}
}