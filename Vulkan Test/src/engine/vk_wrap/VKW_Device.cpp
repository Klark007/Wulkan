#include "VKW_Device.h"

VKW_Device::VKW_Device(std::shared_ptr<VKW_Instance> instance, std::shared_ptr<VKW_Surface> surface, std::vector<const char*> device_extensions, Required_Device_Features required_features, bool build)
	: selector {instance->get_vkb_instance()}
{
	selector.set_surface(surface->get_surface())
		.add_required_extensions(device_extensions)
		.set_required_features(required_features.rf)
		.set_required_features_11(required_features.rf11)
		.set_required_features_12(required_features.rf12)
		.set_required_features_13(required_features.rf13);

	// prefered type gets ignored unless we set allow any gpu to false
	// makes non discrete gpu to be only be partially suitable
	// suitable devices are preferred over partially suitable ones
	selector.allow_any_gpu_device_type(false);
	selector.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

	if (build)
		select_and_build();
}

VKW_Device::~VKW_Device()
{
	vkb::destroy_device(device);
}

void VKW_Device::select_and_build()
{
	// prefers dedicated gpu and should check for correct swapchain support
	auto selection_result = selector.select();

	if (!selection_result) {
		throw SetupException("Failed to select device: " + selection_result.error().message(), __FILE__, __LINE__);
	}

	physical_device = selection_result.value();

	vkb::DeviceBuilder builder{ physical_device };

	auto build_result = builder.build();

	if (!build_result) {
		throw SetupException("Failed to create device: " + build_result.error().message(), __FILE__, __LINE__);
	}

	device = build_result.value();
}
