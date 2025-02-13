#include "VKW_Instance.h"

// set_debug_callback, ...

void VKW_Instance::init(const std::string& app_name, std::vector<const char*> instance_extensions, std::vector<const char*> instance_layers)
{
	auto system_info_ret = vkb::SystemInfo::get_system_info();
	if (!system_info_ret) {
		throw SetupException("Failed to query system info: " + system_info_ret.error().message(), __FILE__, __LINE__);
	}

	// check validation layer
	vkb::SystemInfo system_info = system_info_ret.value();
	if (enable_validation_layers && !system_info.validation_layers_available) {
		throw SetupException("Validation layers not available", __FILE__, __LINE__);
	}

	// check instance layers
	for (const char* layer : instance_layers) {
		if (!system_info.is_layer_available(layer)) {
			throw SetupException("Requested instance layer " + std::string(layer) + " is not available", __FILE__, __LINE__);
		}
	}

	// check extensions
	for (const char* extension : instance_extensions) {
		if (!system_info.is_extension_available(extension)) {
			throw SetupException("Requested instance extension " + std::string(extension) + " is not available", __FILE__, __LINE__);
		}
	}

	vkb::InstanceBuilder builder;
	builder.set_app_name(app_name.c_str())
		.require_api_version(1, 3, 0)
		.enable_extensions(instance_extensions);

	for (const char* layer : instance_layers) {
		builder.enable_layer(layer);
	}

	if (enable_validation_layers) {
		builder.enable_validation_layers();
		
		builder.add_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		);

		builder.add_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		);

		builder.use_default_debug_messenger();
	}

	auto build_result = builder.build();

	if (!build_result) {
		throw SetupException("Failed to create instance: " + build_result.error().message(), __FILE__, __LINE__);
	}

	vkb_instance = build_result.value();

	// load required functions
	volkLoadInstance(vkb_instance.instance);
}

void VKW_Instance::del()
{
	vkb::destroy_instance(vkb_instance);
}