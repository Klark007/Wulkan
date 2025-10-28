#pragma once
#include "common.h"
#include "ObjMesh.h"

struct InstanceData {
	alignas(16) glm::vec3 position;
};

// could also just inherit ObjMesh and make set up less cumbersome
class InstancedShape : public ObjMesh
{
public:
	void init(const VKW_Device& device, const VKW_CommandPool& graphics_pool, const VKW_CommandPool& transfer_pool, VKW_DescriptorPool& descriptor_pool, RenderPass<PushConstants, PBR_MAT_DESC_SET_COUNT>& render_pass, const VKW_Path& obj_path, const std::vector<InstanceData>& per_instance_data, const VKW_Path& mtl_path="");
	void del() override;

	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame) override;
private:

	VKW_Buffer instance_buffer;
public:
};

// overhead cost of virtual function call
inline void InstancedShape::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame)
{
	ObjMesh::draw(command_buffer, current_frame);
}
