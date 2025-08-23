#pragma once

#include "Mesh.h"

class ObjMesh : public Shape
{
public:
	ObjMesh() = default;
	void init(const VKW_Device& device, const VKW_CommandPool& transfer_pool, const std::string& obj_path, const std::string& mtl_path="./models/");
	void del() override;

	// goes over all materials in obj and renders them, expects to be in active command buffer
	// TODO: Current assumption is that all materials in ObjMesh use the same pipeline
	inline void draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline) override;
protected:
	std::vector<Mesh> meshes; // one mesh per material
	VKW_Buffer vertex_buffer;
	VkDeviceAddress vertex_buffer_address;
};


inline void ObjMesh::draw(const VKW_CommandBuffer& command_buffer, uint32_t current_frame, const VKW_GraphicsPipeline& pipeline)
{
}