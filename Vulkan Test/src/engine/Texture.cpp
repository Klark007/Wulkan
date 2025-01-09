#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "Texture.h"

#include <format>


void Texture::init(const VKW_Device* vkw_device, unsigned int w, unsigned int h, VkFormat f, VkImageUsageFlags usage, SharingInfo sharing_info)
{
	device = vkw_device;
	allocator = device->get_allocator();
	width = w;
	height = h;
	format = f;

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1; // 2d

	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;

	image_info.format = format;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL; // Not readable by cpu without changing Tiling

	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	
	// todo start exclusive, if transfered into: change ownership during layout transition
	image_info.sharingMode = sharing_info.mode;
	if (image_info.sharingMode & VK_SHARING_MODE_CONCURRENT) {
		image_info.pQueueFamilyIndices = sharing_info.queue_families.data();
		image_info.queueFamilyIndexCount = sharing_info.queue_families.size();
	}

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

	VmaAllocationInfo alloc_info;

	VK_CHECK_ET(vmaCreateImage(allocator, &image_info, &alloc_create_info, &image, &allocation, &alloc_info), RuntimeException, "Failed to allocate image");
	memory = alloc_info.deviceMemory;
}

void Texture::del()
{
	for (auto& [_, view] : image_views) {
		VK_DESTROY(view, vkDestroyImageView, *device, view);
	}

	if (image && allocation && memory) {
		vmaDestroyImage(allocator, image, allocation);
		image = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}
}

VkImageView Texture::get_image_view(VkImageAspectFlags aspect_flag)
{
	if (image_views.contains(aspect_flag)) {
		return image_views[aspect_flag];
	} else {
		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = format;

		view_info.subresourceRange.aspectMask = aspect_flag;

		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
	
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		VkImageView image_view;
		VK_CHECK_ET(vkCreateImageView(*device, &view_info, nullptr, &image_view), RuntimeException, std::format("Failed to create image view for aspect {}", aspect_flag));


		image_views[aspect_flag] = image_view;
		return image_view;
	}
}

void Texture::transition_layout(const VKW_CommandPool* command_pool, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership, uint32_t new_ownership)
{
	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true
	);
	command_buffer.begin_single_use();

	transition_layout(command_buffer, image, initial_layout, new_layout, old_ownership, new_ownership);

	command_buffer.submit_single_use();
}

void Texture::transition_layout(const VKW_CommandBuffer& command_buffer, VkImage image, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership, uint32_t new_ownership)
{
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = image;
	barrier.oldLayout = initial_layout;
	barrier.newLayout = new_layout;

	// potentially change ownership
	barrier.srcQueueFamilyIndex = old_ownership;
	barrier.dstQueueFamilyIndex = new_ownership;

	// change for whole image
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkImageAspectFlags aspect = 0;
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.aspectMask = aspect;

	// todo: cases to support:

	// for textures:
	// transition from undefined to transfer dst
	// transition from transfer dst to read only

	// for images as render targets:
	// transition from undefined to color attachement
	// transition from color attachement to read only

	// for depth and or stencil:
	// transition from undefined to depth/stencil attachment
	// from depth/stencil to read only (if we do z pre pass)

	if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR; // needs to be in new layout before transfer stage

		barrier.srcAccessMask = VK_ACCESS_2_NONE; // nothing from before as data is undefined
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
			| VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT
			| VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT; // textures can be read in fragment and tesselation shaders

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	}
	else {
		// does a full stop
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
	}

	VkDependencyInfo depency_info{};
	depency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

	depency_info.pImageMemoryBarriers = &barrier;
	depency_info.imageMemoryBarrierCount = 1;

	vkCmdPipelineBarrier2(command_buffer, &depency_info);

}
