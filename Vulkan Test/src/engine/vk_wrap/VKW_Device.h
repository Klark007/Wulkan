#pragma once

#include "../vk_types.h"

#include "VKW_Instance.h"
#include "VKW_Surface.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"

struct Required_Device_Features {
	VkPhysicalDeviceFeatures rf;
	VkPhysicalDeviceVulkan11Features rf11;
	VkPhysicalDeviceVulkan12Features rf12;
	VkPhysicalDeviceVulkan13Features rf13;
};

class VKW_Device
{
public:
	// if we want to require extension features, we need to set build to false and call build before using the VKW_Device
	// get_dedicated_queue vs get_queue  if no dedicated one was found
	VKW_Device(std::shared_ptr<VKW_Instance> instance, std::shared_ptr<VKW_Surface> surface, std::vector<const char*> device_extensions, Required_Device_Features required_features, bool build=true);
	~VKW_Device();
private:
	std::shared_ptr<VKW_Instance> instance;

	vkb::PhysicalDevice physical_device;
	vkb::Device device;

	vkb::PhysicalDeviceSelector selector;

	VmaAllocator allocator;
	void init_allocator();
public:
	template<class T>
	void add_extension_features(T features);

	void select_and_build();

	inline vkb::Device get_vkb_device() const { return device; };
	inline VkDevice get_device() const { return device.device; };
	inline operator VkDevice() const { return device.device; };
	inline vkb::PhysicalDevice get_vkb_physical_device() const { return physical_device; };
	inline VkPhysicalDevice get_physical_device() const { return physical_device.physical_device; };

	inline VmaAllocator get_allocator() const { return allocator; };
};

// Not sure if working correctly
template<class T>
inline void VKW_Device::add_extension_features(T features)
{
	selector.add_required_extension_features(features);
}
