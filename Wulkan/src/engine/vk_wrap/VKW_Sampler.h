#pragma once

#include "../vk_types.h"
#include "VKW_Object.h"

#include "VKW_Device.h"
#include "../Texture.h"

class VKW_Sampler : public VKW_Object
{
public:
	VKW_Sampler();
	void init(const VKW_Device* vkw_device, const std::string& obj_name);
	void del() override;
private:
	const VKW_Device* device;
	std::string name;
	VkSampler sampler;

	// reasonable defaults which can be overloaded before calling init
	VkFilter min_filter;
	VkFilter mag_filter;

	// behavior outside address range
	VkSamplerAddressMode address_mode; // same for U,V ( & W)

	float max_anisotropy; // most modern machines support up to 16, is clamped to maxAnisotropy (VkPhysicalDeviceProperties)
	bool anisotropic_sampling;
public:
	// call before init to change default configuration
	void set_min_filter(VkFilter filter) { min_filter = filter; };
	// call before init to change default configuration
	void set_mag_filter(VkFilter filter) { mag_filter = filter; };
	// call before init to change default configuration
	void set_address_mode(VkSamplerAddressMode mode) { address_mode = mode; };
	// call before init to change default configuration
	void set_anisotropic_sampling(bool state) { anisotropic_sampling = state; };
	// call before init to change default configuration
	void set_anisotropy(float max) { max_anisotropy = max; };

	VkSampler get_sampler() const { return sampler; };
	inline operator VkSampler() const { return sampler; };
};

