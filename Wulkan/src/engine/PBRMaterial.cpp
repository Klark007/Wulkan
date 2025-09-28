#include "PBRMaterial.h"

void PBRMaterial::init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass)
{
	MaterialInstance< PushConstants, PBR_MAT_DESC_SET_COUNT>::init(
		device,
		descriptor_pool,
		render_pass
	);
}

void PBRMaterial::del()
{
	MaterialInstance< PushConstants, PBR_MAT_DESC_SET_COUNT>::del();
}