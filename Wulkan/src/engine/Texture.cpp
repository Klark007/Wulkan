#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "Texture.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include "spdlog/spdlog.h"

void Texture::init(const VKW_Device* vkw_device, unsigned int w, unsigned int h, VkFormat f, VkImageUsageFlags usage, SharingInfo sharing_info, const std::string& obj_name, VkImageCreateFlags flags, uint32_t array_layers, uint32_t mip_levels)
{
	device = vkw_device;
	allocator = device->get_allocator();
	width = w;
	height = h;
	format = f;
	name = obj_name;
	m_mip_levels = mip_levels;

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.flags = flags;
	
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1; // 2d

	image_info.mipLevels = m_mip_levels;
	image_info.arrayLayers = array_layers;

	image_info.format = format;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL; // Not readable by cpu without changing Tiling

	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	
	// todo start exclusive, if transfered into: change ownership during layout transition
	image_info.sharingMode = sharing_info.mode;
	if (image_info.sharingMode & VK_SHARING_MODE_CONCURRENT) {
		image_info.pQueueFamilyIndices = sharing_info.queue_families.data();
		image_info.queueFamilyIndexCount = static_cast<uint32_t>(sharing_info.queue_families.size());
	}

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

	VmaAllocationInfo alloc_info;

	VK_CHECK_ET(vmaCreateImage(allocator, &image_info, &alloc_create_info, &image, &allocation, &alloc_info), RuntimeException, fmt::format("Failed to allocate image ({})", name));
	
#ifdef TRACY_ENABLE
	tracy_mem_instance_id = buffer_instance_count++;
	TracyAllocN(reinterpret_cast<void*>(tracy_mem_instance_id), alloc_info.size, "Textures");
#endif
	
	memory = alloc_info.deviceMemory;

	device->name_object((uint64_t)image, VK_OBJECT_TYPE_IMAGE, name);
}

void Texture::del()
{
	for (auto& [_, view] : image_views) {
		VK_DESTROY(view, vkDestroyImageView, *device, view);
	}

	if (image && allocation && memory) {
		vmaDestroyImage(allocator, image, allocation);

#ifdef TRACY_ENABLE
		TracyFreeN(reinterpret_cast<void*>(tracy_mem_instance_id), "Textures");
#endif

		image = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}
}

VkImageView Texture::get_image_view(VkImageAspectFlags aspect_flag, VkImageViewType type, uint32_t base_layer, uint32_t array_layers)
{
	std::tuple<VkImageAspectFlags, VkImageViewType, int> tuple = std::make_tuple(aspect_flag, type, base_layer);
	if (image_views.contains(tuple)) {
		return image_views[tuple];
	} else {
		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image;
		view_info.viewType = type;
		view_info.format = format;

		view_info.subresourceRange.aspectMask = aspect_flag;

		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = m_mip_levels;

		view_info.subresourceRange.baseArrayLayer = base_layer;
		view_info.subresourceRange.layerCount = array_layers;

		VkImageView image_view;
		VK_CHECK_ET(vkCreateImageView(*device, &view_info, nullptr, &image_view), RuntimeException, fmt::format("Failed to create image ({}) view for aspect {}", name, aspect_flag));
		device->name_object((uint64_t)image_view, VK_OBJECT_TYPE_IMAGE_VIEW, name + " view");

		image_views[tuple] = image_view;
		return image_view;
	}
}

void Texture::transition_layout(const VKW_CommandPool* command_pool, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership, uint32_t new_ownership, uint32_t mip_level, uint32_t level_count)
{
	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true,
		"Layout Transition Single Use CMD"
	);
	command_buffer.begin_single_use();

	transition_layout(command_buffer, image, initial_layout, new_layout, old_ownership, new_ownership, mip_level, level_count);

	command_buffer.submit_single_use();
}

void Texture::transition_layout(const VKW_CommandBuffer& command_buffer, VkImage image, VkImageLayout initial_layout, VkImageLayout new_layout, uint32_t old_ownership, uint32_t new_ownership, uint32_t mip_level, uint32_t level_count)
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
	barrier.subresourceRange.baseMipLevel = mip_level;
	barrier.subresourceRange.levelCount = level_count;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkImageAspectFlags aspect = 0;
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || initial_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
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

	if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		// new texture to be used as color attachment
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR;

		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
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
	else if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
		// new texture to be used as a depth attachment
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		// read from depth attachment in a shader
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
		// general layout supports all operations
		// Note: This should however only be used in special purposes (eg texture is used in compute shader, otherwise try more specific approach)

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
	}
	else if (initial_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		// general layout supports all operations
		// Note: This should however only be used in special purposes (eg texture is used in compute shader, otherwise try more specific approach)
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

		barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
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

void Texture::copy(const VKW_CommandBuffer& command_buffer, const Texture& src_texture, VkImageAspectFlags aspect)
{
	copy(command_buffer, src_texture.image, image, { src_texture.width, src_texture.height }, { width, height }, aspect);
}

void Texture::copy(const VKW_CommandBuffer& command_buffer, VkImage src_texture, VkImage dst_texture, VkExtent2D src_size, VkExtent2D dst_size, VkImageAspectFlags aspect, uint32_t src_mip_level, uint32_t dst_mip_level)
{
	// Blit is less restrictive than copy image
	VkBlitImageInfo2 blit_info{};
	blit_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;

	blit_info.srcImage = src_texture;
	blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	blit_info.dstImage = dst_texture;
	blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	blit_info.filter = VK_FILTER_LINEAR;

	VkImageBlit2 blit_region{};
	blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	// bounds of regions, srcOffsets[0] are 0 initialized
	blit_region.srcOffsets[1].x = src_size.width;
	blit_region.srcOffsets[1].y = src_size.height;
	blit_region.srcOffsets[1].z = 1;

	blit_region.dstOffsets[1].x = dst_size.width;
	blit_region.dstOffsets[1].y = dst_size.height;
	blit_region.dstOffsets[1].z = 1;

	blit_region.srcSubresource.aspectMask = aspect;
	blit_region.srcSubresource.baseArrayLayer = 0;
	blit_region.srcSubresource.layerCount = 1;
	blit_region.srcSubresource.mipLevel = src_mip_level;

	blit_region.dstSubresource = blit_region.srcSubresource;
	blit_region.dstSubresource.mipLevel = dst_mip_level;

	blit_info.pRegions = &blit_region;
	blit_info.regionCount = 1;

	vkCmdBlitImage2(command_buffer, &blit_info);
}

float* load_exr_image(const VKW_Path& path, int& width, int& height, int& channels) {
	channels = 4;
	
	float* rgba;
	const char* err = nullptr;
	int res = LoadEXRWithLayer(&rgba, &width, &height, path.string().c_str(), nullptr, &err);
	
	if (res) {
		std::string msg = fmt::format("Failed to koad EXR file ({}) at {}", res, path);
		if (err) {
			msg += std::string(": ") + err;
			FreeEXRErrorMessage(err);
		}

		throw IOException(msg, __FILE__, __LINE__);
	}

	return rgba;
}

stbi_uc* load_image(const VKW_Path& path, int& width, int& height, int& channels, VkFormat format) {
	channels = Texture::get_stbi_channels(format);

	int ch;
	stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &ch, channels);

	if (!pixels) {
		std::string reason = std::string(stbi_failure_reason());
		throw IOException(fmt::format("Failed to load image ({}) at {}", reason, path), __FILE__, __LINE__);
	}

	return pixels;
}

// helper function for create_*_from_path
VkBufferImageCopy create_buffer_image_copy(VkDeviceSize buffer_offset, uint32_t mip_level, uint32_t base_array_level, unsigned int width, unsigned int height) {
	VkBufferImageCopy image_copy{};
	image_copy.bufferOffset = buffer_offset;

	image_copy.bufferRowLength = 0; // tightly packed
	image_copy.bufferImageHeight = 0;

	image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_copy.imageSubresource.mipLevel = mip_level;
	image_copy.imageSubresource.baseArrayLayer = base_array_level;
	image_copy.imageSubresource.layerCount = 1;

	image_copy.imageOffset = { 0,0,0 };
	image_copy.imageExtent = { width, height, 1 };
	return image_copy;
}

Texture create_texture_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, const VKW_Path& path, Texture_Type type, const std::string& name) {
	VkFormat format = Texture::find_format(*device, type);

	bool is_exr = path.extension().string() == ".exr";

	VKW_Buffer staging_buffer;
	int width, height;
	if (is_exr) {
		int channels;
		
		float* rgba = load_exr_image(path, width, height, channels);
		
		VkDeviceSize image_size = width * height * channels * sizeof(float);
		staging_buffer = create_staging_buffer(device, image_size, rgba, image_size, "EXR staging buffer");

		free(rgba);
	} else {
		int channels;
		stbi_uc* pixels = load_image(path, width, height, channels, format);

		VkDeviceSize image_size = width * height * channels * sizeof(stbi_uc);
		staging_buffer = create_staging_buffer(device, image_size, pixels, image_size, "Image staging buffer");
	
		stbi_image_free(pixels);
	}

	Texture texture{};
	texture.init(
		device,
		width, height,
		format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(), // exclusively owned by graphics queue
		name
	);


	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true,
		"Image creation CMD"
	);

	command_buffer.begin_single_use();

	// transfer layout 1
	Texture::transition_layout(
		command_buffer,
		texture,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	// copying
	VkBufferImageCopy image_copy = create_buffer_image_copy(0, 0, 0, (unsigned int) width, (unsigned int) height);

	vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

	// transfer layout 2
	Texture::transition_layout(
		command_buffer,
		texture,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	command_buffer.submit_single_use();

	staging_buffer.del();

	return texture;
}

Texture create_cube_map_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, const VKW_Path& path, Texture_Type type, const std::string& name)
{
	VkFormat format = Texture::find_format(*device, type);

	bool is_exr = path.extension().string() == ".exr";

	// get all paths
	std::string path_str = path.string();
	size_t special_symbol_idx = path_str.find("%");
	if (special_symbol_idx == std::string::npos) {
		throw IOException(
			fmt::format("Cube map path does not contain % to be replaced with sides (+X,-X,...): {}", path), __FILE__, __LINE__
		);
	}

	std::string pre_string = path_str.substr(0, special_symbol_idx);
	std::string post_string = path_str.substr(special_symbol_idx+1, path_str.size() - (special_symbol_idx + 1));
	std::array<VKW_Path, 6> paths = {
		pre_string + "+X" + post_string,
		pre_string + "-X" + post_string,		

		pre_string + "+Y" + post_string,
		pre_string + "-Y" + post_string,

		pre_string + "-Z" + post_string, // vulkan cube maps are y down
		pre_string + "+Z" + post_string,
	};

	VKW_Buffer staging_buffer{};
	VkDeviceSize image_size;

	int width, height;
	if (is_exr) {
		int channels;

		float* rgba = load_exr_image(paths[0], width, height, channels);

		image_size = width * height * channels * sizeof(float);
		staging_buffer = create_staging_buffer(device, image_size*6, rgba, image_size, "Cube map staging buffer");
		staging_buffer.map();

		free(rgba);
	}
	else {
		throw NotImplementedException("Creating cube map with low dynamic range images is not supported", __FILE__, __LINE__);
	}

	// copy all 6 images into single staging buffer
	for (unsigned int i = 1; i < 6; i++) {
		int channels;

		float* rgba = load_exr_image(paths[i], width, height, channels);

		staging_buffer.copy(rgba, image_size, image_size * i);

		free(rgba);
	}
	staging_buffer.unmap();


	Texture texture{};
	texture.init(
		device,
		width, height,
		format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(), // exclusively owned by graphics queue
		name,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		6
	);

	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true,
		"Cubemap creation CMD"
	);

	command_buffer.begin_single_use();
	{
		// transfer layout 1
		Texture::transition_layout(
			command_buffer,
			texture,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		std::array<VkBufferImageCopy, 6> image_copies{};

		for (unsigned int i = 0; i < 6; i++) {
			image_copies[i] = create_buffer_image_copy(
				image_size * i, // buffer offset
				0, // mip level
				i, // array level
				(unsigned int) width,
				(unsigned int) height
			);
		}

		vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, image_copies.data());

		// transfer layout 2
		Texture::transition_layout(
			command_buffer,
			texture,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
	command_buffer.submit_single_use();
	
	staging_buffer.del();

	return texture;
}

Texture create_mipmapped_texture_from_path(const VKW_Device* device, const VKW_CommandPool* command_pool, const VKW_Path& path, Texture_Type type, const std::string& name)
{
	VkFormat format = Texture::find_format(*device, type);

	bool is_exr = path.extension().string() == ".exr";

	// TODO Exr mipmap
	if (is_exr) {
		throw NotImplementedException("Creating mipmapped texture not supported for high dynamic range", __FILE__, __LINE__);
	}

	std::string path_str = path.string();
	size_t special_symbol_idx = path_str.find(".");

	VKW_Path path_mip_0{ path_str.substr(0, special_symbol_idx) + "_0" + path.extension().string()};

	VKW_Buffer staging_buffer; // if already computed miplevels: all mip levels into said stating buffer, else only layer 0
	std::vector<uint32_t> staging_offsets{}; // stores offsets into staging buffer (only used if loaded from existing images)
	int width, height;
	uint32_t mip_levels;

	bool load_mip_maps = std::filesystem::exists(path_mip_0);

	if (load_mip_maps) {
		// load image 0
		int channels;

		std::vector<stbi_uc*> pixel_mips{}; // stores pointers to images
		std::vector<uint32_t> image_sizes{}; // stores the size of each mip level image
		uint32_t current_size = 0;

		pixel_mips.push_back(load_image(path_mip_0, width, height, channels, format));
		image_sizes.push_back(static_cast<uint32_t>(width * height * channels * sizeof(stbi_uc)));

		staging_offsets.push_back(current_size);
		current_size += image_sizes[0];

		mip_levels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1);

		// load stored images
		for (unsigned int i = 1; i < mip_levels; i++) {
			VKW_Path path_mip_i{ fmt::format("{}_{}{}", path_str.substr(0, special_symbol_idx), i, path.extension().string()) };

			int mip_width, mip_height, mip_channels;

			if (!std::filesystem::exists(path_mip_i)) {
				throw IOException(
					fmt::format("Mipmap image: {} does not exists even though level 0 does", path_mip_i), __FILE__, __LINE__
				);
			}

			pixel_mips.push_back(load_image(path_mip_i, mip_width, mip_height, mip_channels, format));

			if (channels != mip_channels) {
				throw IOException(
					fmt::format("Mipmap images (level: {}) has {} channels, while mipmap level 0 has {}", i, mip_channels, channels), __FILE__, __LINE__
				);
			}
			if (mip_width != (width >> i)) {
				throw IOException(
					fmt::format("Mipmap images (level: {}) has width {} and not expected width {}", i, mip_channels, mip_width, (width >> i)), __FILE__, __LINE__
				);
			}
			if (mip_height != (height >> i)) {
				throw IOException(
					fmt::format("Mipmap images (level: {}) has height {} and not expected height {}", i, mip_channels, mip_height, (height >> i)), __FILE__, __LINE__
				);
			}

			image_sizes.push_back(static_cast<uint32_t>(mip_width * mip_height * mip_channels * sizeof(stbi_uc)));
			staging_offsets.push_back(current_size);
			current_size += image_sizes[i];
		}

		VkDeviceSize image_size = current_size;
		staging_buffer = create_staging_buffer(device, image_size, pixel_mips[0], image_sizes[0], "Image staging buffer");

		// copy images into staging buffer
		staging_buffer.map();
		for (unsigned int i = 1; i < mip_levels; i++) {
			staging_buffer.copy(pixel_mips[i], image_sizes[i], staging_offsets[i]);
		}
		staging_buffer.unmap();

		for (unsigned int i = 0; i < mip_levels; i++) {
			stbi_image_free(pixel_mips[i]);
		}
	}
	else {
		// dynamic mip map generation
		// create image with first level
		if (is_exr) {
			int channels;

			float* rgba = load_exr_image(path, width, height, channels);

			VkDeviceSize image_size = width * height * channels * sizeof(float);
			staging_buffer = create_staging_buffer(device, image_size, rgba, image_size, "EXR staging buffer");

			free(rgba);
		}
		else {
			int channels;
			stbi_uc* pixels = load_image(path, width, height, channels, format);

			VkDeviceSize image_size = width * height * channels * sizeof(stbi_uc);
			staging_buffer = create_staging_buffer(device, image_size, pixels, image_size, "Image staging buffer");

			stbi_image_free(pixels);
		}

		mip_levels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1);
	}

	Texture texture{};
	texture.init(
		device,
		width, height,
		format,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		sharing_exlusive(), // exclusively owned by graphics queue
		name,
		0, // no special flags
		1, // not texture array
		mip_levels
	);

	VKW_CommandBuffer command_buffer{};
	command_buffer.init(
		device,
		command_pool,
		true,
		"Mipmap creation CMD"
	);

	command_buffer.begin_single_use();
	{
		// transfer layout 1
		Texture::transition_layout(
			command_buffer,
			texture,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		if (load_mip_maps) {
			// copies
			std::vector<VkBufferImageCopy> image_copies{};

			for (unsigned int i = 0; i < mip_levels; i++) {
				image_copies.push_back(
					create_buffer_image_copy(
						staging_offsets[i], // buffer offsetm
						i, // mip level
						0, // array kevek
						((unsigned int)width) >> i, ((unsigned int)height) >> i // width / height
					)
				);
			}

			vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(image_copies.size()), image_copies.data());
		}
		else {
			// copy staging buffer into mip level 0
			VkBufferImageCopy image_copy = create_buffer_image_copy(0, 0, 0, (unsigned int) width, (unsigned int) height);

			vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
			
			for (unsigned int i = 1; i < mip_levels; i++) {
				Texture::transition_layout(
					command_buffer,
					texture,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					i-1,
					1
				);

				Texture::copy(command_buffer, texture.get_image(), texture.get_image(), { ((unsigned int)width) >> (i - 1), ((unsigned int)height) >> (i - 1) }, { ((unsigned int)width) >> i, ((unsigned int)height) >> i }, VK_IMAGE_ASPECT_COLOR_BIT, i - 1, i);
			}

			Texture::transition_layout(
				command_buffer,
				texture,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				mip_levels - 1,
				1
			);
		}

		// transfer layout 2
		Texture::transition_layout(
			command_buffer,
			texture,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	command_buffer.submit_single_use();

	staging_buffer.del();

	return texture;
}
