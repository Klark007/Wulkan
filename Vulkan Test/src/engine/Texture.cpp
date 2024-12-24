#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "Texture.h"

#include <format>


Texture::Texture(std::shared_ptr<VKW_Device> d, unsigned int w, unsigned int h, VkFormat f, VkImageUsageFlags usage, SharingInfo sharing_info)
	: device {d}, allocator{device->get_allocator()}, width {w}, height {h}, format {f}
{
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

Texture::~Texture()
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