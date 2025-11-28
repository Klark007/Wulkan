#include "common.h"
#include "VKW_Queue.h"

void VKW_Queue::init(const VKW_Device& device, vkb::QueueType type, const std::string& obj_name)
{
	name = obj_name;

	vkb::Device vkb_device = device.get_vkb_device();
	vkb::Result<VkQueue> queue_result {{}};
	vkb::Result<uint32_t> idx_result {{}};

	// if device has dedicated queue, use that
	if ((type == vkb::QueueType::compute && vkb_device.physical_device.has_dedicated_compute_queue())
	 || (type == vkb::QueueType::transfer && vkb_device.physical_device.has_dedicated_transfer_queue())) {
		queue_result = vkb_device.get_dedicated_queue(type);
		idx_result = vkb_device.get_dedicated_queue_index(type);
	}
	else {
		queue_result = vkb_device.get_queue(type);
		idx_result = vkb_device.get_queue_index(type);
	}
	
	if (!queue_result) {
		throw SetupException(fmt::format("Failed to select a queue ({}): {}", name, queue_result.error().message()), __FILE__, __LINE__);
	}
	if (!idx_result) {
		throw SetupException(fmt::format("Failed to select a queue family ({}): {}", name, idx_result.error().message()), __FILE__, __LINE__);
	}

	queue = queue_result.value();
	family_idx = idx_result.value();

	device.name_object((uint64_t)queue, VK_OBJECT_TYPE_QUEUE, name);
}
