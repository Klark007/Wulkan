#pragma once

#include "vk_types.h"

#include <stb_image.h>

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
class Texture
{
public:
	// create an image (and memory) but not load it (to be used as attachment), if we want to load an image, use create_texture_from_path
	Texture(std::shared_ptr<VKW_Device> device, unsigned int width, unsigned int height, VkFormat format, VkImageUsageFlags usage, SharingInfo sharing_info);
	
	~Texture();
private:
	std::shared_ptr<VKW_Device> device;
	VmaAllocator allocator;
	VmaAllocation allocation;

	VkFormat format;
	
	std::map<VkImageAspectFlags, VkImageView> image_views;
public:	// todo remove
	VkImage image;
	VkDeviceMemory memory;

	unsigned int width, height;

private: // todo remove
	inline static std::vector<VkFormat> potential_formats(Texture_Type type);
	inline static VkFormatFeatureFlags required_format_features(Texture_Type type);

public:
	VkImageView get_image_view(VkImageAspectFlags aspect_flag);

	inline static VkFormat find_format(std::shared_ptr<VKW_Device> device, Texture_Type type);
	inline static int get_stbi_channels(Texture_Type type);
};

// todo: only return texture, move to Texture.cpp
inline std::pair<std::shared_ptr<Texture>, std::shared_ptr<VKW_Buffer>> create_texture_from_path(std::shared_ptr<VKW_Device> device, std::string path, Texture_Type type, SharingInfo sharing_info) {
	int desired_channels = Texture::get_stbi_channels(type);

	int width, height, channels;
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, desired_channels); // force rgba for texture STBI_grey

	if (!pixels) {
		std::string reason = std::string(stbi_failure_reason());
		throw IOException(std::format("Failed to load image: {}", reason), __FILE__, __LINE__);
	}

	VkDeviceSize image_size = width * height * desired_channels;
	// todo make staging buffer local to constructor
	std::shared_ptr<VKW_Buffer> staging_buffer = create_staging_buffer(device, pixels, image_size);

	stbi_image_free(pixels);

	VkFormat format = Texture::find_format(device, type);
	std::shared_ptr<Texture> texture = std::make_shared<Texture>(
		device, 
		width, height, 
		format, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		sharing_info
	);

	// transfer and copy

	return std::pair(texture, staging_buffer);
}

inline VkFormat Texture::find_format(std::shared_ptr<VKW_Device> device, Texture_Type type)
{
	VkFormatFeatureFlags format_features = required_format_features(type);
	std::vector<VkFormat> candidates = potential_formats(type);

	VkPhysicalDevice p_device = device->get_physical_device();

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