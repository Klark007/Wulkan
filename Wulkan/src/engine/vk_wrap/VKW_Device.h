#pragma once

#include "../common.h"
#include "VKW_Object.h"

#include "VKW_Instance.h"
#include "VKW_Surface.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_DEBUG_LOG_FORMAT
#include "vma/vk_mem_alloc.h"

struct Required_Device_Features {
	VkPhysicalDeviceFeatures rf;
	VkPhysicalDeviceVulkan11Features rf11;
	VkPhysicalDeviceVulkan12Features rf12;
	VkPhysicalDeviceVulkan13Features rf13;
};

class VKW_Device : public VKW_Object
{
public:
	VKW_Device() = default;
	// if we want to require extension features, we need to set build to false and call build before using the VKW_Device
	// get_dedicated_queue vs get_queue  if no dedicated one was found
	void init(VKW_Instance* vkw_instance, const VKW_Surface& surface, const std::vector<const char*>& device_extensions, const Required_Device_Features& required_features, const std::string& obj_name, bool build=true);
	void del() override;
private:
	VKW_Instance* instance;
	std::string m_name;

	vkb::PhysicalDevice physical_device;
	vkb::Device device;

	std::unique_ptr<vkb::PhysicalDeviceSelector> selector;

	VmaAllocator allocator;
	void init_allocator();
public:
	template<class T>
	void add_extension_features(T features);

	void select_and_build();

	inline void name_object(uint64_t object_handle, VkObjectType object_type, const std::string& name) const;

	inline const vkb::Device& get_vkb_device() const { return device; };
	inline VkDevice get_device() const { return device.device; };
	inline operator VkDevice() const { return device.device; };
	inline VkPhysicalDevice get_physical_device() const { return physical_device.physical_device; };

	inline VmaAllocator get_allocator() const { return allocator; };
};

// Not sure if working correctly
template<class T>
inline void VKW_Device::add_extension_features(T features)
{
	selector->add_required_extension_features(features);
}

inline void VKW_Device::name_object(uint64_t object_handle, VkObjectType object_type, const std::string& name) const
{
	VkDebugUtilsObjectNameInfoEXT  name_info = {};
	name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	name_info.objectHandle = object_handle;
	name_info.objectType = object_type;
	name_info.pObjectName = name.c_str();

#ifndef NDEBUG
	vkSetDebugUtilsObjectNameEXT(device, &name_info);
#endif
}