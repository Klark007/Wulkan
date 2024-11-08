#pragma once

#include "vk_types.h"

#include <stb_image.h>

enum Texture_Channels
{
	Tex_R,
	Tex_RGB,
	Tex_RGBA
};

// Texture with managing VkImage, VkImageView and Memory
// TODO: If necessary split VkImage and VkImage view into its own class stored with smart pointer
class Texture
{
public:
	Texture(std::string path, Texture_Channels channels = Tex_RGBA);
	// Either compute from an image path or just create an image memory but not load it (to be used as attachment)
	
	inline static int get_stbi_channels(Texture_Channels tc);
	inline static VkFrontFace get_vk_format(Texture_Channels tc);
};

