#pragma once

#include <array>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "VulkanTools.h"
#include "RendererStructs.h"
#include "VulkanDevice.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanShader.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanTexture.hpp"
#include "VulkanDescriptor.hpp"

class Scene;

class VulkanRenderer
{
public:
	VulkanRenderer(bool enableValidationLayer, std::shared_ptr<Scene> scene);
	VulkanRenderer(const VulkanRenderer& other) = default; //copy
	VulkanRenderer(VulkanRenderer&& other) = default; //move
	VulkanRenderer& operator=(const VulkanRenderer&) = default;
	VulkanRenderer& operator=(VulkanRenderer&&) = default;
	~VulkanRenderer();

	void InitVulkanRenderer(GLFWwindow* window);
	void UpdateGlobalRenderResources(GlobalVariableUniformBuffer globalData, std::vector<PointLight> lightData);
	void RenderFrame();
	void CleanVulkanResources();

	//void InitSwapchain();
	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateDepthResources();

	void BuildCommandBuffers();
	void ReBuildCommandBuffers();
	
	void CreatePrimaryCommandBuffer(); //testing out multiple command buffers

	void CreateCommandBuffers();
	void CreateSemaphores();

	void PrepareDMACommandBuffer();
	void SubmitDMACommandBuffer();

	void PrepareFrame();
	void SubmitFrame();

	void PrepareResources();

	std::shared_ptr<VulkanDescriptor> GetVulkanDescriptor();
	std::vector<VkDescriptorSetLayoutBinding> GetGloablBindings();
	std::vector<DescriptorPoolSize> GetGlobalPoolSize();
	std::vector<DescriptorUse> GetGlobalDescriptorUses();

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	void SaveScreenshotNextFrame();
	void SetWireframe(bool wireframe);

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VulkanShader shaderManager;
	VulkanPipeline pipelineManager;
	VulkanTextureManager textureManager;

	std::shared_ptr<Scene> scene;
private:

	VmaBuffer globalVariableBuffer;
	DescriptorResource globalVariableBufResource;
	VmaBuffer lightsInfoBuffer;
	DescriptorResource lightsResource;

	//uint32_t frameIndex = 1; // which frame of the swapchain it is on

	VulkanTextureDepthBuffer depthBuffer;

	//Command buffer per frame
	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	std::vector<std::shared_ptr<VulkanDescriptor>> descriptors;

	uint32_t frameIndex; //which of the swapchain images the app is rendering to
	bool wireframe = false; //whether or not to use the wireframe pipeline for the scene.
	bool saveScreenshot = false;

	VkClearColorValue clearColor = {{ 0.2f, 0.3f, 0.3f, 1.0f }};
	VkClearDepthStencilValue depthClearColor = { 0.0f, 0 };

	std::array<VkClearValue, 2> GetFramebufferClearValues();

	void SaveScreenshot();
};

