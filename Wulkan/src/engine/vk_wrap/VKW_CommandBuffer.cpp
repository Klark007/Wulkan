#include "VKW_CommandBuffer.h"

void VKW_CommandBuffer::init(const VKW_Device* vkw_device, const VKW_CommandPool* vkw_command_pool, bool su)
{
	device = vkw_device;
	command_pool = vkw_command_pool;
	queue = vkw_command_pool->get_queue();
	single_use = su;

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = *command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // can be submitted directly to queue but not called from other command buffers
	alloc_info.commandBufferCount = 1;

	VK_CHECK_ET(vkAllocateCommandBuffers(*device, &alloc_info, &command_buffer), SetupException, "Failed to create Command buffer");
}

void VKW_CommandBuffer::begin_single_use()
{
	if (!single_use) {
		throw SetupException("Tried to begin single use with buffer not created for single use", __FILE__, __LINE__);
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_ET(vkBeginCommandBuffer(command_buffer, &begin_info), RuntimeException, "Failed to begin single use command buffer");
}

void VKW_CommandBuffer::submit_single_use()
{
	if (!single_use) {
		throw SetupException("Tried to end single use with buffer not created for single use", __FILE__, __LINE__);
	}

	VK_CHECK_ET(vkEndCommandBuffer(command_buffer), RuntimeException, "Failed to end one time command buffer");

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.commandBufferCount = 1;

	VK_CHECK_ET(vkQueueSubmit(*queue, 1, &submit_info, VK_NULL_HANDLE), RuntimeException, "Failed to submit one time command buffer");
	vkQueueWaitIdle(*queue);

	VK_DESTROY_FROM(command_buffer, vkFreeCommandBuffers, *device, *command_pool, 1, &command_buffer);
}

void VKW_CommandBuffer::begin() const
{
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK_ET(vkBeginCommandBuffer(command_buffer, &begin_info), RuntimeException, "Failed to begin recording command buffer");
}

void VKW_CommandBuffer::submit(const std::vector<VkSemaphore>& wait_semaphores, const std::vector<VkPipelineStageFlags>& wait_stages, const std::vector<VkSemaphore>& signal_semaphores, VkFence fence) const
{
	VK_CHECK_ET(vkEndCommandBuffer(command_buffer), RuntimeException, "Failed to record command buffer");

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

	
	VK_CHECK_ET(vkQueueSubmit(*queue, 1, &submit_info, fence), RuntimeException, "Failed to submit command buffer");
}