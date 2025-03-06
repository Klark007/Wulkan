#pragma once

#include "vk_wrap/VKW_Object.h"
#include "vk_wrap/VKW_CommandBuffer.h"
#include "vk_wrap/VKW_GraphicsPipeline.h"

class Shape : public VKW_Object {
public:
	inline virtual void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) = 0;
	virtual void del() override = 0;
protected:
	glm::mat4 model;
	int cascade_idx;
public:
	void set_model_matrix(const glm::mat4& m) { model = m; };
	void set_cascade_idx(int idx) { cascade_idx = idx; };
};