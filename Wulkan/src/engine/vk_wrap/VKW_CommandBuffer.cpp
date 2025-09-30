#include "VKW_CommandBuffer.h"

void VKW_CommandBuffer::init(const VKW_Device* vkw_device, const VKW_CommandPool* vkw_command_pool, bool su, const std::string& obj_name)
{
	device = vkw_device;
	name = obj_name;
	command_pool = vkw_command_pool;
	queue = vkw_command_pool->get_queue();
	single_use = su;

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = *command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // can be submitted directly to queue but not called from other command buffers
	alloc_info.commandBufferCount = 1;

	VK_CHECK_ET(vkAllocateCommandBuffers(*device, &alloc_info, &command_buffer), SetupException, fmt::format("Failed to create Command buffer ({})", name));
	device->name_object((uint64_t)command_buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}

void VKW_CommandBuffer::begin_single_use()
{
	if (!single_use) {
		throw SetupException(fmt::format("Tried to begin single use with buffer ({}) not created for single use", name), __FILE__, __LINE__);
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_ET(vkBeginCommandBuffer(command_buffer, &begin_info), RuntimeException, fmt::format("Failed to begin single use command buffer ({})", name));
}

void VKW_CommandBuffer::submit_single_use()
{
	if (!single_use) {
		throw SetupException(fmt::format("Tried to end single use with buffer ({}) not created for single use", name), __FILE__, __LINE__);
	}

	VK_CHECK_ET(vkEndCommandBuffer(command_buffer), RuntimeException, fmt::format("Failed to end one time command buffer ({})", name));

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.commandBufferCount = 1;

	VK_CHECK_ET(vkQueueSubmit(*queue, 1, &submit_info, VK_NULL_HANDLE), RuntimeException, fmt::format("Failed to submit one time command buffer ({})", name));
	vkQueueWaitIdle(*queue);

	VK_DESTROY_FROM(command_buffer, vkFreeCommandBuffers, *device, *command_pool, 1, &command_buffer);
}

void VKW_CommandBuffer::begin() const
{
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK_ET(vkBeginCommandBuffer(command_buffer, &begin_info), RuntimeException, fmt::format("Failed to begin recording command buffer ({})", name));
}

void VKW_CommandBuffer::submit(const std::vector<VkSemaphore>& wait_semaphores, const std::vector<VkPipelineStageFlags>& wait_stages, const std::vector<VkSemaphore>& signal_semaphores, VkFence fence) const
{
	VK_CHECK_ET(vkEndCommandBuffer(command_buffer), RuntimeException, fmt::format("Failed to record command buffer ({})", name));

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.commandBufferCount = 1;

	if (wait_semaphores.size() != wait_stages.size()) {
		throw SetupException("Not every wait semaphore has a corresponding wait stage", __FILE__, __LINE__);
	}

	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.pWaitDstStageMask = wait_stages.data();
	submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());

	submit_info.pSignalSemaphores = signal_semaphores.data();
	submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());

	
	VK_CHECK_ET(vkQueueSubmit(*queue, 1, &submit_info, fence), RuntimeException, fmt::format("Failed to submit command buffer", name));
}