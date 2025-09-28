#pragma once

#include "Material.h"
#include "Renderpass.h"
constexpr size_t PBR_MAT_DESC_SET_COUNT = 3;

class PBRMaterial : public MaterialInstance< PushConstants, PBR_MAT_DESC_SET_COUNT>{
public:
	PBRMaterial() = default;

	void init(const VKW_Device& device, const VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass);

	void del() override;
};