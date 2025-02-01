#pragma once

#include "../vk_types.h"
#include "VKW_Object.h"

#include "VKW_Device.h"
#include "VKW_Shader.h"
#include "VKW_DescriptorSet.h"
#include "VKW_PushConstants.h"

class VKW_GraphicsPipeline : public VKW_Object {
public:
	VKW_GraphicsPipeline();
	void init(const VKW_Device* vkw_device);
	void del() override;
	void clear();
private:
	const VKW_Device* device;
	VkPipeline graphics_pipeline;
	VkPipelineLayout layout;

	VkExtent2D render_extent;

	// the following fields can be configured (before calling init)
	VkPipelineInputAssemblyStateCreateInfo input_assembly; // define topology such as triangles, points, lines
	VkPipelineRasterizationStateCreateInfo rasterizer; // rasterization state such as depth bias, wireframe, backface culling
	VkPipelineDepthStencilStateCreateInfo depth_stencil; // depth and stencil testing
	bool tesselation_enabled;
	VkPipelineTessellationStateCreateInfo tesselation; // tesselation state
	VkPipelineMultisampleStateCreateInfo multisampling; // multisample state for multisample anti aliasing
	VkPipelineColorBlendAttachmentState color_blending_attachement; // color blending for transparancy etc.

	VkPipelineRenderingCreateInfo render_info; // used for dynamic rendering
	VkFormat color_attachment_format;
	VkFormat depth_attachment_format;

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages; // shaders used in the pipeline

	VkPipelineLayoutCreateInfo pipeline_layout_info; // layout for descriptor sets and push constants to be used in the pipeline
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
	std::vector<VkPushConstantRange> push_consts_range;

	VkRenderingInfo attachment_state; // used for begin rendering, storing the area to be rendered into
	std::unique_ptr<VkRenderingAttachmentInfo> color_attachment_info; // defines load ops (happen before first access) and store op (after last) for color attachment
	std::unique_ptr<VkRenderingAttachmentInfo> depth_attachment_info;  // defines load ops (happen before first access) and store op (after last) for depth attachment
	
	VkViewport viewport;
	VkRect2D scissor;

	// Todo functionality for stencil clear
public:
	inline void bind(const VKW_CommandBuffer& cmd) const { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline); };

	// begins render pass using the attachment state of the pipeline
	//inline void begin_rendering(const VKW_CommandBuffer& cmd) const { vkCmdBeginRendering(cmd, &attachment_state); };
	void begin_rendering(const VKW_CommandBuffer& cmd) const;

	// ends the current render pass
	inline void end_rendering(const VKW_CommandBuffer& cmd) const { vkCmdEndRendering(cmd); };


	// BEGIN TO BE SET BEFORE INIT
	// set topology to be rendered (or input into the tesselation stage if enabled
	inline void set_topology(VkPrimitiveTopology topology);
	
	// set normal (VK_POLYGON_MODE_FILL), wireframe mode
	inline void set_wireframe_mode(bool use_wireframe=true);
	// set culling settings
	inline void set_culling_mode(VkCullModeFlags culling_mode=VK_CULL_MODE_BACK_BIT, VkFrontFace front_face=VK_FRONT_FACE_COUNTER_CLOCKWISE);
	
	// enable depth testing (by default closer values are smaller, not always preferred: see https://developer.nvidia.com/blog/visualizing-depth-precision/)
	inline void enable_depth_test(VkCompareOp compare_op = VK_COMPARE_OP_LESS);
	// enables writing to depth buffer
	inline void enable_depth_write();

	inline void enable_tesselation() { tesselation_enabled = true;	};
	// sets size of patches inputed into tesselation
	inline void set_tesselation_patch_size(uint32_t patch_size);

	inline void set_rasterization_samples(VkSampleCountFlagBits samples);
	// TODO add sampleShadingEnable to also support MSAA in textures not just geometry

	// set formats of color attachments used in this pipeline
	inline void set_color_attachment_format(VkFormat format);
	// set formats of depth attachments used in this pipeline
	inline void set_depth_attachment_format(VkFormat format);

	// adds VKW_Shader to the pipeline
	void add_shader_stages(const std::vector<VKW_Shader>& stages);

	// adds Descriptorset to the pipeline
	void add_descriptor_sets(const std::vector<VKW_DescriptorSetLayout>& layouts);

	// adds a pushconstant to the pipeline
	template <class T>
	inline void add_push_constant(VKW_PushConstant<T> push_const);
	void add_push_constants(const std::vector<VkPushConstantRange>& ranges);
	
	// END TO BE SET BEFORE INIT
	
	void set_color_attachment(VkImageView attachment, bool do_clear_color, VkClearColorValue clear_color_value);
	void set_depth_attachment(VkImageView attachment, bool do_clear_depth, float clear_depth_value);
	
	void set_render_size(VkExtent2D extend);

	// sets the dynamic viewport (with extend set by set_render_size), expects to be in an active render pass
	// Todo could later also set min and max depth for inverted zbuffer
	inline void set_dynamic_viewport(const VKW_CommandBuffer& cmd) const { vkCmdSetViewport(cmd, 0, 1, &viewport);  };
	// sets the dynamic scissor (with extend set by set_render_size), expects to be in an active render pass
	inline void set_dynamic_scissor(const VKW_CommandBuffer& cmd) const { vkCmdSetScissor(cmd, 0, 1, &scissor); };

	inline operator VkPipeline() const { return graphics_pipeline; };
	inline VkPipeline get_pipeline() const { return graphics_pipeline; };
	inline VkPipelineLayout get_layout() const { return layout; };
};

inline void VKW_GraphicsPipeline::set_topology(VkPrimitiveTopology topology)
{
	input_assembly.topology = topology;
	input_assembly.primitiveRestartEnable = VK_FALSE; // might need to be enabled for line and triangle strips
}

inline void VKW_GraphicsPipeline::set_wireframe_mode(bool use_wireframe)
{
	if (use_wireframe) {
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	}
	else {
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	}
}

inline void VKW_GraphicsPipeline::set_culling_mode(VkCullModeFlags culling_mode, VkFrontFace front_face)
{
	rasterizer.cullMode = culling_mode;
	rasterizer.frontFace = front_face;
}

inline void VKW_GraphicsPipeline::enable_depth_test(VkCompareOp compare_op)
{
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthCompareOp = compare_op;
}

inline void VKW_GraphicsPipeline::enable_depth_write()
{
	depth_stencil.depthWriteEnable = VK_TRUE;
}

inline void VKW_GraphicsPipeline::set_tesselation_patch_size(uint32_t patch_size)
{
	tesselation.patchControlPoints = patch_size;
}

inline inline void VKW_GraphicsPipeline::set_rasterization_samples(VkSampleCountFlagBits samples)
{
	multisampling.rasterizationSamples = samples;
}

inline void VKW_GraphicsPipeline::set_color_attachment_format(VkFormat format)
{
	color_attachment_format = format;

	render_info.pColorAttachmentFormats = &color_attachment_format;
	render_info.colorAttachmentCount = 1;
}

inline void VKW_GraphicsPipeline::set_depth_attachment_format(VkFormat format)
{
	depth_attachment_format = format;

	render_info.depthAttachmentFormat = format;
}

template<class T>
inline void VKW_GraphicsPipeline::add_push_constant(VKW_PushConstant<T> push_const)
{
	push_consts_range.push_back(push_const.get_range());
}