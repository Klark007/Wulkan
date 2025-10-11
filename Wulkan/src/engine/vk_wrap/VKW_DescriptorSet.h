#pragma once

#include "../common.h"

#include "VKW_Device.h"
#include "VKW_Object.h"
#include "VKW_DescriptorPool.h"

#include "VKW_Buffer.h"
#include "../Texture.h"
#include "VKW_Sampler.h"
#include "VKW_CommandBuffer.h"

class VKW_DescriptorSetLayout : public VKW_Object {
public:
	VKW_DescriptorSetLayout() = default;

	void init(const VKW_Device* device, const std::string& obj_name);
	void del() override;
private:
	const VKW_Device* device = nullptr;
	std::string name;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;

	std::vector<VkDescriptorSetLayoutBinding> bindings;
public:
	void add_binding(uint32_t binding_slot, VkDescriptorType type, VkShaderStageFlags shader_stages);
	inline VkDescriptorSetLayout get_layout() const { return layout; };
	inline operator VkDescriptorSetLayout() const { return layout; };
	const std::vector<VkDescriptorSetLayoutBinding>& get_bindings() const { return bindings; };
};

// Todo: could expand into array of descriptor sets in one class
class VKW_DescriptorSet : public VKW_Object
{
public:
	VKW_DescriptorSet() = default;

	void init(const VKW_Device* device, VKW_DescriptorPool* pool, const VKW_DescriptorSetLayout& layout, const std::string& obj_name);
	// bind descriptor set to bind point, assumes to be recording commands into the above command buffer
	void bind(const VKW_CommandBuffer& command_buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t set_idx=0) const;
	void del() override;
private:
	const VKW_Device* device = nullptr;
	std::string name;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set;

	// stores mapping from binding (unique per set) to descriptor type
	std::map<uint32_t, VkDescriptorType> binding_types;
public:
	// Updates descriptor at binding. Assumes binding corresponds to a type which can be updated using a buffer
	void update(uint32_t binding, const VKW_Buffer& buffer) const;

	// Updates descriptor at binding. Assumes binding corresponds to a combined sampler and the image corresponding to the view is in the mentioned layout 
	void update(uint32_t binding, VkImageView image_view, const VKW_Sampler& sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

	// Updates descriptor at binding. Assumes binding corresponds to a image corresponding to the view is in the mentioned layout 
	// see: https://stackoverflow.com/questions/77070602/how-are-separated-sampled-images-and-samplers-used-in-vulkan-with-glsl
	// be careful with sampler*Shadow as Shadow is not part of the type
	void update(uint32_t binding, VkImageView image_view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

	// Updates descriptor at binding. Assumes binding corresponds to a sampler
	void update(uint32_t binding, const VKW_Sampler& sampler) const;


	inline VkDescriptorSet get_descriptor_set() const { return descriptor_set; };
	inline operator VkDescriptorSet() const { return descriptor_set; };

};

