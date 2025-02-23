#pragma once

#include "common.h"

#include <stb_image.h>
#include "vk_wrap/VKW_Object.h"

#include "vk_wrap/VKW_Device.h"
#include "vk_wrap/VKW_Buffer.h"

// texture type defines the potential formats, need to check if formats have requested features
enum Texture_Type
{
	Tex_DS,  // create depth & stencil texture
	Tex_R,   // create R only texture (sRGB, conversion to linear done)
	Tex_R_Linear,   // create R only texture without sRGB to linear conversion of Tex_R
	Tex_RGB, // create RGB texture
	Tex_RGB_Linear, // create RGB texture without sRGB to linear conversion
	Tex_RGBA, // create RGBA texture
	Tex_HDR_RGBA, // create RGBA with high dynamic range texture

	Tex_Colortarget, // creates target used for rendering 
	Tex_D,   // create depth texture
	Tex_Float // single channel float for computations such as curvature
};

// Texture with managing VkImage, VkImageView and Memory
// TODO: If necessary split VkImage and VkImage view into its own class stored with smart pointer
class Texture : public VKW_Object
{
public:
	Texture() = default;
	// create an image (and memory) but not load it (to be used as attachment), if we want to load an image, use create_texture_from_path
	void init(const VKW_Device* vkw_device, unsigned int width, unsigned int height, VkFormat format, VkImageUsageFlags usage, SharingInfo sharing_info, const std::string& obj_name, VkImageCreateFlags flags = 0, uint32_t array_layers = 1);
	void del() override;

private:
	const VKW_Device* device;
	std::string name;
	VmaAllocator allocator;
	VmaAllocation allocation;
	
	VkImage image;
	VkDeviceMemory memory;

	std::map<std::pair<VkImageAspectFlags, VkImageViewType>, VkImageView> image_views;

	unsigned int width, height;
	VkFormat format;

	inline static std::vector<VkFormat> potential_formats(Texture_Type type);
	inline static VkFormatFeatureFlags required_format_features(Texture_Type type);
public:
	// transitions the layout. Can also be used to change ownership to a new queue
	void transition_layout(const VKW_CommandPool* command_pool, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership = VK_QUEUE_FAMILY_IGNORED, uint32_t new_ownership = VK_QUEUE_FAMILY_IGNORED);
	// transitions layout. In contrast to the above function this is done in a currently active command buffer
	static void transition_layout(const VKW_CommandBuffer& command_buffer, VkImage image, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership = VK_QUEUE_FAMILY_IGNORED, uint32_t new_ownership = VK_QUEUE_FAMILY_IGNORED);

	// copies from src texture into this texture,
	// assumes to be in an active command buffer, and that 
	// src_texture has layout of VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and that 
	// this texture has layout of VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	void copy(const VKW_CommandBuffer& command_buffer, const Texture& src_texture, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
	// copies from src Texture into dest Texture
	// assumes to be in an active command buffer, and that 
	// src_texture has layout of VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and that 
	// this texture has layout of VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	static void copy(const VKW_CommandBuffer& command_buffer, VkImage src_texture, VkImage dst_texture, VkExtent2D src_size, VkExtent2D dst_size, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

	inline static VkFormat find_format(const VKW_Device& device, Texture_Type type);
	inline static int get_stbi_channels(VkFormat format);

	// gets image view of aspect with specific type (assumes one view per aspect flag, image view type combiniation)
	VkImageView get_image_view(VkImageAspectFlags aspect_flag, VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D, uint32_t array_layers = 1);

	inline VkImage get_image() const { return image; };
	inline operator VkImage() const { return image; };
	inline VkFormat get_format() const { return format; };
	inline unsigned int get_width() const { return width; };
	inline unsigned int get_height() const { return height; };
	inline VkExtent2D get_extent() const { return { width,height }; };
};


// creates a texture from a path, needs graphics command pool as input argument as we are waiting on a stage not present supported in transfer queues (in transition_layout)
// Low-dynamic range images are created with the nr channels dictated by the type (1,3,4)
// High dynamic range images are always 4 channels
Texture create_texture_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, const std::string& path, Texture_Type type, const std::string& name);

// create a cube map from a path containing a %. % sign will be replaced with (+|-) (X|Y|Z) to get the 6 faces
// Currently only supports hdr images (exr files)
Texture create_cube_map_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, const std::string& path, Texture_Type type, const std::string& name);

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

inline int Texture::get_stbi_channels(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8_SRGB:
	case VK_FORMAT_R8_UNORM:
		return STBI_grey;
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_R8G8B8_UNORM:
		return STBI_rgb;
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_R8G8B8A8_UNORM:
		return STBI_rgb_alpha;
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	default:
		throw NotImplementedException(std::format("Unknown type {:x}", static_cast<int>(format)), __FILE__, __LINE__);
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
	case Tex_R_Linear:
		return { VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
	case Tex_RGB:
		return { VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
	case Tex_RGB_Linear:
		return { VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
	case Tex_RGBA:
		return { VK_FORMAT_R8G8B8A8_SRGB };
	case Tex_HDR_RGBA:
		return { VK_FORMAT_R32G32B32A32_SFLOAT };
	case Tex_Colortarget:
		return { VK_FORMAT_R16G16B16A16_SFLOAT };
	case Tex_Float:
		return { VK_FORMAT_R16_SFLOAT };
	default:
		throw NotImplementedException(std::format("Unknown type {}", (unsigned int) type), __FILE__, __LINE__);
	}
}

inline VkFormatFeatureFlags Texture::required_format_features(Texture_Type type) {
	switch (type)
	{
	case Tex_D:
	case Tex_DS:
		return VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	case Tex_R:
	case Tex_R_Linear:
	case Tex_RGB:
	case Tex_RGB_Linear:
	case Tex_RGBA:
	case Tex_HDR_RGBA:
		return VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	case Tex_Float:
	case Tex_Colortarget:
		return
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
			VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
			VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
			VK_FORMAT_FEATURE_TRANSFER_SRC_BIT
			;
	default:
		throw NotImplementedException(std::format("Unknown type {}", (unsigned int) type), __FILE__, __LINE__);
	}
}