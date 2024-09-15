#include "VKW_CommandBuffer.h"

VKW_CommandBuffer::VKW_CommandBuffer(std::shared_ptr<VKW_Device> device, std::shared_ptr<VKW_CommandPool> command_pool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool->get_command_pool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // can be submitted directly to queue but not called from other command buffers
	allocInfo.commandBufferCount = 1;

	VK_CHECK_ET(vkAllocateCommandBuffers(device->get_device(), &allocInfo, &command_buffer), SetupException, "Failed to create Command buffer");
}
