#pragma once
#include <iostream>

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#include <vulkan\vulkan.h>
#include "VulkanDevice.hpp"

#include "..\core\Texture.h"

#include "..\third-party\stb_image\stb_image.h"

class VulkanTexture {
public:
	VulkanDevice *device;

	VkImage textureImage;
	VkImageLayout textureImageLayout;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkDescriptorImageInfo descriptor;

	int mipLevels;

	void updateDescriptor();

	void destroy();

	void createImageSampler(VkFilter mag = VK_FILTER_LINEAR, VkFilter min = VK_FILTER_LINEAR, VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float mipMapLodBias = 0.0f, bool useStaging = 1, bool anisotropy = 1, float maxAnisotropy = 8,
		VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

};

/**
* Load a 2D texture including all mip levels
*
* @param filename File to load (supports .ktx and .dds)
* @param format Vulkan format of the image data stored in the file
* @param device Vulkan device to create the texture on
* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
*
*/
class VulkanTexture2D : public VulkanTexture {
public:
	Texture* texture;

	void loadFromTexture(
		Texture* texture,
		VkFormat format,
		VulkanDevice *device,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true);
};

class VulkanTexture2DArray : public VulkanTexture {
public:
	TextureArray* textures;

	/**
	* Load a 2D texture array including all mip levels
	*
	* @param filename File to load (supports .ktx and .dds)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void loadTextureArray(
		TextureArray* textures,
		VkFormat format,
		VulkanDevice *device,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanCubeMap : public VulkanTexture {
public:
	CubeMap* cubeMap;

	/**
	* Load a cubemap texture including all mip levels from a single file
	*
	* @param filename File to load (supports .ktx and .dds)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void loadFromTexture(
		CubeMap* cubeMap,
		VkFormat format,
		VulkanDevice *device,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};
