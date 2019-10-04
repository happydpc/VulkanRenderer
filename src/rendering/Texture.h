#pragma once

#include <cstddef>
#include <memory>
#include <thread>
#include <vulkan/vulkan.h>

#include "AsyncTask.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Wrappers.h"

#include "resources/Texture.h"
#include "vk_mem_alloc.h"

class VulkanDevice;

struct TexCreateDetails
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	bool genMipMaps = false;
	int mipMapLevelsToGen = 1;
	int desiredWidth = 0;
	int desiredHeight = 0;

	TexCreateDetails (){};
	TexCreateDetails (VkFormat format,
	    VkImageLayout imageLayout,
	    bool genMipMaps,
	    int mipMapLevelsToGen,
	    int desiredWidth = 0,
	    int desiredHeight = 0)
	: format (format),
	  imageLayout (imageLayout),
	  genMipMaps (genMipMaps),
	  mipMapLevelsToGen (mipMapLevelsToGen),
	  desiredWidth (desiredWidth),
	  desiredHeight (desiredHeight){};
};

namespace details
{
struct TexData
{
	VulkanDevice* device;

	VkImageLayout textureImageLayout;

	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
	VmaAllocator allocator = nullptr;

	int mipLevels;
	int layers;
};
} // namespace details
class VulkanTexture
{
	public:
	VulkanTexture (VulkanDevice& device,
	    AsyncTaskManager& async_task_man,
	    std::function<void()> const& finish_work,
	    TexCreateDetails texCreateDetails,
	    Resource::Texture::TexResource textureResource);

	VulkanTexture (VulkanDevice& device,
	    AsyncTaskManager& async_task_man,
	    std::function<void()> const& finish_work,
	    TexCreateDetails texCreateDetails,
	    std::shared_ptr<VulkanBuffer> buffer);

	VulkanTexture (VulkanDevice& device, TexCreateDetails texCreateDetails);

	~VulkanTexture ();

	VulkanTexture (VulkanTexture const& tex) = delete;
	VulkanTexture& operator= (VulkanTexture const& tex) = delete;

	VulkanTexture (VulkanTexture&& tex);
	VulkanTexture& operator= (VulkanTexture&& tex);

	DescriptorResource resource;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;

	private:
	details::TexData data;

	void InitImage2D (VkImageCreateInfo imageInfo);

	VkSampler CreateImageSampler (VkFilter mag = VK_FILTER_LINEAR,
	    VkFilter min = VK_FILTER_LINEAR,
	    VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	    float mipMapLodBias = 0.0f,
	    bool useMipmaps = true,
	    int mipLevels = 1,
	    bool anisotropy = true,
	    float maxAnisotropy = 8,
	    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageView CreateImageView (VkImage image,
	    VkImageViewType viewType,
	    VkFormat format,
	    VkImageAspectFlags aspectFlags,
	    VkComponentMapping components,
	    int mipLevels,
	    int layers);
};

using VulkanTextureID = int;

class TextureManager
{
	public:
	TextureManager (Resource::Texture::Manager& texture_manager,
	    VulkanDevice& device,
	    AsyncTaskManager& async_task_manager,
	    BufferManager& buf_man);
	~TextureManager ();

	TextureManager (TextureManager const& buf) = delete;
	TextureManager& operator= (TextureManager const& buf) = delete;


	VulkanTextureID CreateTexture2D (Resource::Texture::TexID texture, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTexture2DArray (Resource::Texture::TexID textures, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateCubeMap (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTextureFromBuffer (std::shared_ptr<VulkanBuffer> buffer, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateDepthImage (VkFormat depthFormat, int width, int height);

	void delete_texture (VulkanTextureID id);


	VulkanTexture const& get_texture (VulkanTextureID id);

	private:
	std::function<void()> CreateFinishWork (VulkanTextureID id);
	void FinishTextureCreation (VulkanTextureID id);

	VulkanDevice& device;
	Resource::Texture::Manager& texture_manager;
	AsyncTaskManager& async_task_manager;
	BufferManager& buffer_manager;

	VulkanTextureID id_counter = 0;
	std::mutex map_lock;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> in_progress_map;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> texture_map;
};
