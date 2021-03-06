#include "Texture.h"

#include <algorithm>

#include "stb_image.h"

#include "core/Logger.h"

#include "AsyncTask.h"
#include "Buffer.h"
#include "Device.h"
#include "RenderTools.h"
#include "Wrappers.h"
#include "rendering/Initializers.h"


void SetImageLayout (VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = initializers::image_memory_barrier ();
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask =
			    imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}
	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier (cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

// Fixed sub resource on first mip level and layer
void SetImageLayout (VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	SetImageLayout (cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}

void GenerateMipMaps (VkCommandBuffer cmdBuf,
    VkImage image,
    VkImageLayout finalImageLayout,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    uint32_t layers,
    uint32_t mipLevels)
{
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and
	// sample that one down

	// Copy down mips from n-1 to n
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		VkOffset3D srcOffset = { static_cast<int32_t> (width >> (i)), static_cast<int32_t> (height >> (i - 1)), 1 };
		VkOffset3D dstOffset = {
			static_cast<int32_t> (width >> (i - 1)), static_cast<int32_t> (height >> (i)), 1
		};
		VkImageBlit imageBlit = initializers::image_blit (
		    initializers::image_subresource_layers (VK_IMAGE_ASPECT_COLOR_BIT, i - 1, layers, 0),
		    srcOffset,
		    initializers::image_subresource_layers (VK_IMAGE_ASPECT_COLOR_BIT, i, layers, 0),
		    dstOffset);

		VkImageSubresourceRange mipSubRange =
		    initializers::image_subresource_range_create_info (VK_IMAGE_ASPECT_COLOR_BIT, 1, layers);
		mipSubRange.baseMipLevel = i;

		// Transiston current mip level to transfer dest
		SetImageLayout (cmdBuf,
		    image,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    mipSubRange,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Blit from previous level
		vkCmdBlitImage (
		    cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

		// Transiston current mip level to transfer source for read in next iteration
		SetImageLayout (cmdBuf,
		    image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    mipSubRange,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	VkImageSubresourceRange subresourceRange =
	    initializers::image_subresource_range_create_info (VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layers);

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition
	// all to SHADER_READ

	if (finalImageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		Log.error (fmt::format ("Final image layout Undefined!"));
	SetImageLayout (cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalImageLayout, subresourceRange);
}

void SetLayoutAndTransferRegions (VkCommandBuffer transferCmdBuf,
    VkImage image,
    VkBuffer stagingBuffer,
    const VkImageSubresourceRange subresourceRange,
    std::vector<VkBufferImageCopy> bufferCopyRegions)
{

	SetImageLayout (transferCmdBuf, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	vkCmdCopyBufferToImage (transferCmdBuf,
	    stagingBuffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<uint32_t> (bufferCopyRegions.size ()),
	    static_cast<VkBufferImageCopy*> (bufferCopyRegions.data ()));

	SetImageLayout (
	    transferCmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
}

void BeginTransferAndMipMapGenWork (VulkanDevice& device,
    AsyncTaskQueue& async_task_man,
    std::function<void ()> const& finish_work,
    VulkanBuffer* buffer,
    const VkImageSubresourceRange subresourceRange,
    const std::vector<VkBufferImageCopy> bufferCopyRegions,
    VkImageLayout imageLayout,
    VkImage image,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    uint32_t layers,
    uint32_t mipLevels)
{
	if (device.has_dedicated_transfer ())
	{
		std::function<void (const VkCommandBuffer)> work = [=] (const VkCommandBuffer cmdBuf) {
			SetLayoutAndTransferRegions (cmdBuf, image, buffer->get (), subresourceRange, bufferCopyRegions);

			GenerateMipMaps (cmdBuf, image, imageLayout, width, height, depth, layers, mipLevels);
		};

		AsyncTask task;
		task.work = work;
		task.finish_work = finish_work;

		async_task_man.SubmitTask (std::move (task));
	}
	else
	{
		std::function<void (const VkCommandBuffer)> transferWork = [=] (const VkCommandBuffer cmdBuf) {
			SetLayoutAndTransferRegions (cmdBuf, image, buffer->get (), subresourceRange, bufferCopyRegions);
		};

		auto gen_mips = [&, image, imageLayout, width, height, depth, layers, mipLevels] {
			auto mipMapGenWork = [&, image, imageLayout, width, height, depth, layers, mipLevels] (
			                         const VkCommandBuffer cmdBuf) {
				GenerateMipMaps (cmdBuf, image, imageLayout, width, height, depth, layers, mipLevels);
			};

			AsyncTask task_gen_mips;
			task_gen_mips.work = mipMapGenWork;
			task_gen_mips.finish_work = finish_work;

			async_task_man.SubmitTask (std::move (task_gen_mips));
		};


		AsyncTask task_transfer;
		task_transfer.type = TaskType::transfer;
		task_transfer.work = transferWork;

		task_transfer.finish_work = gen_mips; // recursively submit an async task.
		// did it because deal with semaphores is a pain.

		async_task_man.SubmitTask (std::move (task_transfer));
	}
}

VulkanTexture::VulkanTexture (VulkanDevice& device,
    AsyncTaskQueue& async_task_man,
    std::function<void ()> const& finish_work,
    TexCreateDetails texCreateDetails,
    Resource::Texture::TexResource textureResource)
{
	data.device = &device;
	data.mipLevels = texCreateDetails.genMipMaps ? texCreateDetails.mipMapLevelsToGen : 1;
	data.textureImageLayout = texCreateDetails.imageLayout;
	data.layers = static_cast<uint32_t> (textureResource.dims.size ());
	data.width = texCreateDetails.desiredWidth;
	data.height = texCreateDetails.desiredHeight;

	VkExtent3D imageExtent = { static_cast<uint32_t> (textureResource.dims.at (0).width),
		static_cast<uint32_t> (textureResource.dims.at (0).height),
		static_cast<uint32_t> (textureResource.dims.size ()) };

	VkImageCreateInfo imageCreateInfo = initializers::image_create_info (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    data.mipLevels,
	    data.layers,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    imageExtent,
	    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	if (textureResource.tex_type == Resource::Texture::TextureType::cubemap2D)
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


	staging_buffer = std::make_unique<VulkanBuffer> (
	    device, staging_details (BufferType::staging, textureResource.data.size ()));

	staging_buffer->copy_to_buffer (textureResource.data);

	init_image_2d (imageCreateInfo);

	VkImageSubresourceRange subresourceRange = initializers::image_subresource_range_create_info (
	    VK_IMAGE_ASPECT_COLOR_BIT, data.mipLevels, data.layers);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < data.layers; layer++)
	{
		VkBufferImageCopy bufferCopyRegion = initializers::buffer_image_copy_create (
		    initializers::image_subresource_layers (VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer),
		    { static_cast<uint32_t> (textureResource.dims.at (0).width),
		        static_cast<uint32_t> (textureResource.dims.at (0).height),
		        static_cast<uint32_t> (textureResource.dims.size ()) },
		    offset);
		bufferCopyRegions.push_back (bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += textureResource.dims.at (0).width * textureResource.dims.at (0).height *
		          textureResource.dims.size () * textureResource.dims.at (0).channels;
	}

	BeginTransferAndMipMapGenWork (*data.device,
	    async_task_man,
	    finish_work,
	    staging_buffer.get (),
	    subresourceRange,
	    bufferCopyRegions,
	    texCreateDetails.imageLayout,
	    image,
	    textureResource.dims.at (0).width,
	    textureResource.dims.at (0).height,
	    static_cast<uint32_t> (textureResource.dims.size ()),
	    data.layers,
	    data.mipLevels);

	sampler = create_image_sampler (VK_FILTER_LINEAR,
	    VK_FILTER_LINEAR,
	    VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    texCreateDetails.addressMode,
	    0.0f,
	    true,
	    data.mipLevels,
	    true,
	    8,
	    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageViewType viewType;
	if (textureResource.tex_type == Resource::Texture::TextureType::array1D ||
	    textureResource.tex_type == Resource::Texture::TextureType::array2D ||
	    textureResource.tex_type == Resource::Texture::TextureType::array3D)
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	else if (textureResource.tex_type == Resource::Texture::TextureType::cubemap2D)
	{
		viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	imageView = create_image_view (image,
	    viewType,
	    texCreateDetails.format,
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    data.mipLevels,
	    data.layers);
}

VulkanTexture::VulkanTexture (VulkanDevice& device,
    AsyncTaskQueue& async_task_man,
    std::function<void ()> const& finish_work,
    TexCreateDetails texCreateDetails,
    std::unique_ptr<VulkanBuffer> buffer)
: staging_buffer (std::move (buffer))
{
	data.device = &device;
	data.mipLevels = texCreateDetails.genMipMaps ? texCreateDetails.mipMapLevelsToGen : 1;
	data.textureImageLayout = texCreateDetails.imageLayout;
	data.layers = 1;
	data.width = texCreateDetails.desiredWidth;
	data.height = texCreateDetails.desiredHeight;

	VkExtent3D imageExtent = { texCreateDetails.desiredWidth, texCreateDetails.desiredHeight, 1 };

	VkImageCreateInfo imageCreateInfo = initializers::image_create_info (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    data.mipLevels,
	    data.layers,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    imageExtent,
	    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	init_image_2d (imageCreateInfo);

	VkImageSubresourceRange subresourceRange = initializers::image_subresource_range_create_info (
	    VK_IMAGE_ASPECT_COLOR_BIT, data.mipLevels, data.layers);

	std::vector<VkBufferImageCopy> bufferCopyRegions;

	uint32_t offset = 0;
	for (uint32_t layer = 0; layer < data.layers; layer++)
	{
		VkBufferImageCopy bufferCopyRegion = initializers::buffer_image_copy_create (
		    initializers::image_subresource_layers (VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer),
		    { texCreateDetails.desiredWidth, texCreateDetails.desiredHeight, 1 },
		    static_cast<VkDeviceSize> (offset));
		bufferCopyRegions.push_back (bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += texCreateDetails.desiredWidth * texCreateDetails.desiredHeight * 4;
	}

	BeginTransferAndMipMapGenWork (*data.device,
	    async_task_man,
	    finish_work,
	    staging_buffer.get (),
	    subresourceRange,
	    bufferCopyRegions,
	    texCreateDetails.imageLayout,
	    image,
	    texCreateDetails.desiredWidth,
	    texCreateDetails.desiredHeight,
	    1,
	    data.layers,
	    data.mipLevels);

	sampler = create_image_sampler (VK_FILTER_LINEAR,
	    VK_FILTER_LINEAR,
	    VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    texCreateDetails.addressMode,
	    0.0f,
	    true,
	    data.mipLevels,
	    true,
	    8,
	    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageViewType viewType;
	// if(texCreateDetails.layout == Resource::Texture::TextureType::array1D
	//	|| texCreateDetails.layout == Resource::Texture::TextureType::array1D
	//	|| texCreateDetails.layout == Resource::Texture::TextureType::array1D){
	//	viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	//}
	// else if(texCreateDetails.layout == Resource::Texture::TextureType::cubemap2D){
	//	viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	//}
	// else {
	viewType = VK_IMAGE_VIEW_TYPE_2D;
	//}


	imageView = create_image_view (image,
	    viewType,
	    texCreateDetails.format,
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    data.mipLevels,
	    data.layers);
}

VulkanTexture::VulkanTexture (VulkanDevice& device, TexCreateDetails texCreateDetails)
{
	data.device = &device;
	data.layers = 1;
	data.width = texCreateDetails.desiredWidth;
	data.height = texCreateDetails.desiredHeight;

	bool is_depth_stencil = texCreateDetails.format == VK_FORMAT_D32_SFLOAT ||
	                        texCreateDetails.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
	                        texCreateDetails.format == VK_FORMAT_D24_UNORM_S8_UINT;

	VkImageCreateInfo imageInfo = initializers::image_create_info (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    1,
	    1,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VkExtent3D{ texCreateDetails.desiredWidth, texCreateDetails.desiredHeight, 1 },
	    texCreateDetails.usage | VK_IMAGE_USAGE_SAMPLED_BIT);

	if (is_depth_stencil) imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	data.allocator = data.device->get_image_optimal_allocator ();
	VK_CHECK_RESULT (vmaCreateImage (
	    data.allocator, &imageInfo, &imageAllocCreateInfo, &image, &data.allocation, &data.allocationInfo));


	VkImageAspectFlags flags = is_depth_stencil ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
	                                              VK_IMAGE_ASPECT_COLOR_BIT;

	imageView = VulkanTexture::create_image_view (image,
	    VK_IMAGE_VIEW_TYPE_2D,
	    texCreateDetails.format,
	    flags,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    1,
	    1);

	VkImageSubresourceRange subresourceRange = initializers::image_subresource_range_create_info (flags);

	CommandPool pool (device.device, device.graphics_queue ());
	CommandBuffer cmdBuf (pool);
	cmdBuf.allocate ().begin ();
	SetImageLayout (cmdBuf.get (), image, VK_IMAGE_LAYOUT_UNDEFINED, texCreateDetails.imageLayout, subresourceRange);
	cmdBuf.end ().submit ().wait ();
}


VulkanTexture::~VulkanTexture ()
{
	if (image != VK_NULL_HANDLE) vmaDestroyImage (data.allocator, image, data.allocation);
	if (imageView != VK_NULL_HANDLE) vkDestroyImageView (data.device->device, imageView, nullptr);
	if (sampler != VK_NULL_HANDLE) vkDestroySampler (data.device->device, sampler, nullptr);
}

VulkanTexture::VulkanTexture (VulkanTexture&& tex) noexcept
: image (tex.image), imageView (tex.imageView), sampler (tex.sampler), data (tex.data)
{
	tex.image = VK_NULL_HANDLE;
	tex.imageView = VK_NULL_HANDLE;
	tex.sampler = VK_NULL_HANDLE;
}
VulkanTexture& VulkanTexture::operator= (VulkanTexture&& tex) noexcept
{
	image = tex.image;
	imageView = tex.imageView;
	sampler = tex.sampler;
	data = tex.data;

	tex.image = VK_NULL_HANDLE;
	tex.imageView = VK_NULL_HANDLE;
	tex.sampler = VK_NULL_HANDLE;
	return *this;
}


VkSampler VulkanTexture::create_image_sampler (VkFilter mag,
    VkFilter min,
    VkSamplerMipmapMode mipMapMode,
    VkSamplerAddressMode textureWrapMode,
    float mipMapLodBias,
    bool useMipMaps,
    int mipLevels,
    bool anisotropy,
    float maxAnisotropy,
    VkBorderColor borderColor)
{

	// Create a defaultsampler
	VkSamplerCreateInfo samplerCreateInfo = initializers::sampler_create_info ();
	samplerCreateInfo.magFilter = mag;
	samplerCreateInfo.minFilter = min;
	samplerCreateInfo.mipmapMode = mipMapMode;
	samplerCreateInfo.addressModeU = textureWrapMode;
	samplerCreateInfo.addressModeV = textureWrapMode;
	samplerCreateInfo.addressModeW = textureWrapMode;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = (useMipMaps) ? mipLevels : 0.0f; // Max level-of-detail should match mip level count
	samplerCreateInfo.anisotropyEnable = anisotropy; // Enable anisotropic filtering
	samplerCreateInfo.maxAnisotropy =
	    (anisotropy) ? data.device->phys_device.properties.limits.maxSamplerAnisotropy : 1;
	samplerCreateInfo.borderColor = borderColor;

	VkSampler sampler;
	VK_CHECK_RESULT (vkCreateSampler (data.device->device, &samplerCreateInfo, nullptr, &sampler));

	return sampler;
}

VkImageView VulkanTexture::create_image_view (VkImage image,
    VkImageViewType viewType,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkComponentMapping components,
    uint32_t mipLevels,
    uint32_t layers)
{
	VkImageViewCreateInfo viewInfo = initializers::image_view_create_info ();
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.components = components;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	VkImageView imageView;
	VK_CHECK_RESULT (vkCreateImageView (data.device->device, &viewInfo, nullptr, &imageView));

	return imageView;
}

void VulkanTexture::init_image_2d (VkImageCreateInfo imageInfo)
{

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (imageInfo.tiling == VK_IMAGE_TILING_OPTIMAL)
	{
		data.allocator = data.device->get_image_optimal_allocator ();
	}
	else if (imageInfo.tiling == VK_IMAGE_TILING_LINEAR)
	{
		data.allocator = data.device->get_image_allocator ();
	}
	VK_CHECK_RESULT (vmaCreateImage (
	    data.allocator, &imageInfo, &imageAllocCreateInfo, &image, &data.allocation, &data.allocationInfo));
}

Textures::Textures (Resource::Texture::Textures& textures, VulkanDevice& device, AsyncTaskQueue& async_task_queue)
: textures (textures), device (device), async_task_queue (async_task_queue)
{
}

Textures::~Textures () { Log.debug (fmt::format ("Textures left over {}", texture_map.size ())); }

std::function<void ()> Textures::create_finish_work (VulkanTextureID id)
{
	return [this, id] {
		std::lock_guard guard (map_lock);
		auto it = std::find (expired_textures.begin (), expired_textures.end (), id);
		if (it != expired_textures.end ())
		{
			expired_textures.erase (it);
		}
		else
		{
			auto node = in_progress_map.extract (id);
			if (node)
			{
				texture_map.insert (std::move (node));
			}
		}
	};
}

VulkanTextureID Textures::create_texture_2d (Resource::Texture::TexID texture_id, TexCreateDetails texCreateDetails)
{
	auto finish_work = create_finish_work (id_counter);
	auto& resource = textures.get_tex_resource_by_id (texture_id);
	auto tex = std::make_unique<VulkanTexture> (device, async_task_queue, finish_work, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	in_progress_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID Textures::create_texture_2d_array (Resource::Texture::TexID texture_id, TexCreateDetails texCreateDetails)
{
	auto finish_work = create_finish_work (id_counter);
	auto& resource = textures.get_tex_resource_by_id (texture_id);
	auto tex = std::make_unique<VulkanTexture> (device, async_task_queue, finish_work, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	in_progress_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID Textures::create_cube_map (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails)
{
	auto finish_work = create_finish_work (id_counter);
	auto& resource = textures.get_tex_resource_by_id (cubeMap);
	auto tex = std::make_unique<VulkanTexture> (device, async_task_queue, finish_work, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	in_progress_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID Textures::create_texture_from_buffer (
    std::unique_ptr<VulkanBuffer> buffer, TexCreateDetails texCreateDetails)
{
	auto finish_work = create_finish_work (id_counter);
	auto tex = std::make_unique<VulkanTexture> (
	    device, async_task_queue, finish_work, texCreateDetails, std::move (buffer));
	std::lock_guard guard (map_lock);
	in_progress_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTexture Textures::create_attachment_image (TexCreateDetails texCreateDetails)
{
	return VulkanTexture (device, texCreateDetails);
}

VkDescriptorImageInfo Textures::get_resource (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	if (texture_map.count (id) == 1)
		return texture_map.at (id)->get_resource ();
	else // if(in_progress_map.count (id) == 1)
		return in_progress_map.at (id)->get_resource ();
}

VkDescriptorType Textures::get_descriptor_type (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	return texture_map.at (id)->get_descriptor_type ();
}

void Textures::delete_texture (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	auto it = std::find (expired_textures.begin (), expired_textures.end (), id);
	if (it != expired_textures.end ())
	{
		expired_textures.erase (it);
	}
	else
	{
		if (texture_map.count (id) == 1)
			texture_map.erase (id);
		else if (in_progress_map.count (id) == 1)
			expired_textures.push_back (id);
	}
}

bool Textures::is_finished_transfer (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	return texture_map.count (id) == 1;
}