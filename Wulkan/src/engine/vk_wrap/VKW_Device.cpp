#include "common.h"
#include "VKW_Device.h"

void VKW_Device::init(VKW_Instance* vkw_instance, const VKW_Surface& surface, const std::vector<const char*>& device_extensions, const Required_Device_Features& required_features, const std::string& obj_name, bool build)
{
	instance = vkw_instance;
	m_name = obj_name;
	selector = std::make_unique<vkb::PhysicalDeviceSelector>(instance->get_vkb_instance());

	(*selector).set_surface(surface)
		.add_required_extensions(device_extensions)
		.set_required_features(required_features.rf)
		.set_required_features_11(required_features.rf11)
		.set_required_features_12(required_features.rf12)
		.set_required_features_13(required_features.rf13);

	// prefered type gets ignored unless we set allow any gpu to false
	// makes non discrete gpu to be only be partially suitable
	// suitable devices are preferred over partially suitable ones
	selector->allow_any_gpu_device_type(false);
	selector->prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

	if (build)
		select_and_build();
}

void VKW_Device::del()
{
	if (allocator) {
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}

	vkb::destroy_device(device);
}

void VKW_Device::init_allocator()
{
	VmaVulkanFunctions vulkan_functions = {};
	vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocator_info = {};
	// needed so that vma allocated buffers can be used for vkGetBufferDeviceAddress
	allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; 
	allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
	allocator_info.physicalDevice = get_physical_device();
	allocator_info.device = get_device();
	allocator_info.instance = *instance;
	allocator_info.pVulkanFunctions = &vulkan_functions;

	VK_CHECK_ET(vmaCreateAllocator(&allocator_info, &allocator), SetupException, "Failed to create VMA Allocator");
}

void VKW_Device::select_and_build()
{
	// prefers dedicated gpu and should check for correct swapchain support
	auto selection_result = selector->select();

	if (!selection_result) {
		throw SetupException("Failed to select device: " + selection_result.error().message(), __FILE__, __LINE__);
	}

	physical_device = selection_result.value();

	vkb::DeviceBuilder builder{ physical_device };

	auto build_result = builder.build();

	if (!build_result) {
		throw SetupException(fmt::format("Failed to create device ({}): {}", m_name, build_result.error().message()), __FILE__, __LINE__);
	}

	device = build_result.value();

	init_allocator();

	// for faster device level calls; see: https://github.com/zeux/volk?tab=readme-ov-file#optimizing-device-calls
	volkLoadDevice(device.device);

	name_object((uint64_t)device.device, VK_OBJECT_TYPE_DEVICE, m_name);
}
