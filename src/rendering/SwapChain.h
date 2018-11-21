#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "Device.h"
#include "Initializers.h"

class Window;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};
class FrameGraph;

class VulkanSwapChain
{
	public:
	VulkanSwapChain (VulkanDevice& device, Window& window);
	~VulkanSwapChain ();

	void RecreateSwapChain ();

	void CreateFramebuffers (
	    std::vector<int> order, std::array<VkImageView, 3> depthImageViews, VkRenderPass renderPass);

	static SwapChainSupportDetails querySwapChainSupport (VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	private:
	VulkanDevice& device;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	Window& window;

	SwapChainSupportDetails details;

	void createSwapChain ();
	void createImageViews ();

	void DestroySwapchainResources ();

	VkPresentModeKHR chooseSwapPresentMode ();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat ();
	VkExtent2D chooseSwapExtent ();
};
