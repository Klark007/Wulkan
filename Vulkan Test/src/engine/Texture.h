#pragma once

#include "vk_types.h"

#include <stb_image.h>
#include "vk_wrap/VKW_Object.h"

#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_Buffer.h"

// texture type defines the potential formats, need to check if formats have requested features
enum Texture_Type
{
	Tex_D,   // create depth texture
	Tex_DS,  // create depth & stencil texture
	Tex_R,   // create R only texture
	Tex_RGB, // create RGB texture
	Tex_RGBA // create RGBA texture
};

// Texture with managing VkImage, VkImageView and Memory
// TODO: If necessary split VkImage and VkImage view into its own class stored with smart pointer
class Texture : public VKW_Object
{
public:
	Texture() = default;
	// create an image (and memory) but not load it (to be used as attachment), if we want to load an image, use create_texture_from_path
	void init(const VKW_Device* vkw_device, unsigned int width, unsigned int height, VkFormat format, VkImageUsageFlags usage, SharingInfo sharing_info);
	void del() override;

private:
	const VKW_Device* device;
	VmaAllocator allocator;
	VmaAllocation allocation;
	
	VkImage image;
	VkDeviceMemory memory;

	std::map<VkImageAspectFlags, VkImageView> image_views;

	unsigned int width, height;
	VkFormat format;

	inline static std::vector<VkFormat> potential_formats(Texture_Type type);
	inline static VkFormatFeatureFlags required_format_features(Texture_Type type);
public:
	// transitions the layout. Can also be used to change ownership to a new queue
	void transition_layout(const VKW_CommandPool* command_pool, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership = VK_QUEUE_FAMILY_IGNORED, uint32_t new_ownership = VK_QUEUE_FAMILY_IGNORED);

	inline static VkFormat find_format(const VKW_Device& device, Texture_Type type);
	inline static int get_stbi_channels(Texture_Type type);

	inline VkImage get_image() const { return image; };
	inline operator VkImage() const { return image; };
	VkImageView get_image_view(VkImageAspectFlags aspect_flag);
};

// creates a texture from a path, needs graphics command pool as input argument as we are waiting on a stage not present supported in transfer queues (in transition_layout)
inline Texture create_texture_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, std::string path, Texture_Type type) {
	int desired_channels = Texture::get_stbi_channels(type);

	int width, height, channels;
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, desired_channels); // force rgba for texture STBI_grey

	if (!pixels) {
		std::string reason = std::string(stbi_failure_reason());
		throw IOException(std::format("Failed to load image: {}", reason), __FILE__, __LINE__);
	}

	VkDeviceSize image_size = width * height * desired_channels;
	// todo make staging buffer local to constructor
	VKW_Buffer staging_buffer = create_staging_buffer(device, pixels, image_size);

	stbi_image_free(pixels);

	VkFormat format = Texture::find_format(*device, type);
	Texture texture{};
	texture.init(
		device, 
		width, height, 
		format, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		sharing_exlusive() // exclusively owned by graphics queue
	);

	// transfer layout 1
	texture.transition_layout(command_pool,
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	// copying
	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true
	);
	command_buffer.begin_single_use();

	VkBufferImageCopy image_copy{};
	image_copy.bufferOffset = 0;
	image_copy.bufferRowLength = 0; // tightly packed
	image_copy.bufferImageHeight = 0;

	image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_copy.imageSubresource.mipLevel = 0;
	image_copy.imageSubresource.baseArrayLayer = 0;
	image_copy.imageSubresource.layerCount = 1;

	image_copy.imageOffset = { 0,0,0 };
	image_copy.imageExtent = { (unsigned int) width, (unsigned int) height, 1 };

	vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
	command_buffer.submit_single_use();

	// transfer layout 2
	texture.transition_layout(command_pool,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
	
	staging_buffer.del();

	return texture;
}

inline VkFormat Texture::find_format(const VKW_Device& device, Texture_Type type)
{
	VkFormatFeatureFlags format_features = required_format_features(type);
	std::vector<VkFormat> candidates = potential_formats(type);

	VkPhysicalDevice p_device = device.get_physical_device();

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(p_device, format, &props);

		if ((props.optimalTilingFeatures & format_features) == format_features) {
			return format;
		}
	}
	throw RuntimeException(std::format("No format found for type {}", (unsigned int) type), __FILE__, __LINE__);
}

inline int Texture::get_stbi_channels(Texture_Type type)
{
	switch (type)
	{
	case Tex_D:
	case Tex_DS:
	case Tex_R:
		return STBI_grey;
	case Tex_RGB:
		return STBI_rgb;
	case Tex_RGBA:
		return STBI_rgb_alpha;
	default:
		throw NotImplementedException(std::format("Unknown type {}", (unsigned int) type), __FILE__, __LINE__);
	}
}

inline std::vector<VkFormat> Texture::potential_formats(Texture_Type type)
{
	// TODO: switch to compressed textures
	// see: https://registry.khronos.org/vulkan/specs/1.1/html/vkspec.html#_identification_of_formats
	switch (type)
	{
	case Tex_D:
		return { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	case Tex_DS:
		return { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	case Tex_R:
		return { VK_FORMAT_R8_SRGB, VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
	case Tex_RGB:
		return { VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
	case Tex_RGBA:
		return { VK_FORMAT_R8G8B8A8_SRGB };
	default:
		throw NotImplementedException(std::format("Unknown type {}", (unsigned int) type), __FILE__, __LINE__);
	}
}

inline VkFormatFeatureFlags Texture::required_format_features(Texture_Type type) {
	switch (type)
	{
	case Tex_D:
	case Tex_DS:
		return VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	case Tex_R:
		return VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	case Tex_RGB:
	case Tex_RGBA:
		return 
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | 
			VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | 
			VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
			VK_FORMAT_FEATURE_TRANSFER_DST_BIT
		;
	default:
		throw NotImplementedException(std::format("Unknown type {}", (unsigned int) type), __FILE__, __LINE__);
	}
}