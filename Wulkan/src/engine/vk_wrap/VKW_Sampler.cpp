#include "VKW_Sampler.h"

VKW_Sampler::VKW_Sampler()
	: device {nullptr}, 
	sampler {VK_NULL_HANDLE}, 
	min_filter {VK_FILTER_LINEAR}, 
	mag_filter {VK_FILTER_LINEAR}, 
	address_mode { VK_SAMPLER_ADDRESS_MODE_REPEAT }, 
	max_anisotropy {2.0f},
	anisotropic_sampling {VK_TRUE},
	compare_enable {VK_FALSE}
{ }

void VKW_Sampler::init(const VKW_Device* vkw_device, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	sampler_info.minFilter = min_filter;
	sampler_info.magFilter = mag_filter;

	sampler_info.addressModeU = address_mode;
	sampler_info.addressModeV = address_mode;
	sampler_info.addressModeW = address_mode;
	
	sampler_info.anisotropyEnable = anisotropic_sampling;
	sampler_info.maxAnisotropy = max_anisotropy;

	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // color used in VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
	sampler_info.unnormalizedCoordinates = VK_FALSE; // [0,1] range of coordinates

	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.minLod = 0;
	sampler_info.maxLod = 0;

	sampler_info.compareEnable = compare_enable;
	sampler_info.compareOp = m_compare_op;

	VK_CHECK_ET(vkCreateSampler(*device, &sampler_info, VK_NULL_HANDLE, &sampler), RuntimeException, fmt::format("Failed to create texture sampler ({})", name));
	device->name_object((uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, name);
}

void VKW_Sampler::del()
{
	VK_DESTROY(sampler, vkDestroySampler, *device, sampler);
}
