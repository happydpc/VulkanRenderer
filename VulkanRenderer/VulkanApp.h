#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <string>
#include <array>
#include <set>


#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"

//#include "ImGuiImpl.h"
//#include <imgui.h>
#include "Mesh.h"
#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"

const int WIDTH = 1000;
const int HEIGHT = 800;

const float deltaTime = 0.016f;
struct GlobalVariableUniformBuffer {
	glm::mat4 view;
	glm::mat4 proj;
	float time;
};

struct ModelBufferObject {
	glm::mat4 model;
	glm::mat4 normal;
};

struct PointLight {
	glm::vec4 lightPos = glm::vec4(50.0f, 25.0f, 50.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	PointLight() {};
	PointLight(glm::vec4 pos, glm::vec4 col, glm::vec4 atten) : lightPos(pos), color(col), attenuation(atten) {};
};

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	void initWindow();
	void initVulkan();
	void prepareScene();
	void mainLoop();
	void HandleInputs();
	void drawFrame();
	void cleanup();

	void cleanupSwapChain();
	void recreateSwapChain();
	void reBuildCommandBuffers();

	void MouseMoved(double xpos, double ypos);
	void MouseClicked(int button, int action, int mods);
	void KeyboardEvent(int key, int scancode, int action, int mods);

private:

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	int frameCount = 1;
	
	bool firstMouse;
	double lastX, lastY;
	bool mouseControlEnabled;
	bool keys[512] = {};
	void SetMouseControl(bool value);
	bool wireframe = false;

	void createRenderPass();
	void createDescriptorSetLayout();
	void createPipelineCache();
	void createGraphicsPipelines();
	void createDepthResources();
	void createFramebuffers();

	void createTextureImage(VkImage image, VkDeviceMemory imageMemory);
	void createTextureSampler(VkSampler* textureSampler);
	void createUniformBuffers();

	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSemaphores();

	void updateUniformBuffers();

	void newGuiFrame();
	void updateImGui();
	void prepareImGui();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	int terrainCount = 4;
	std::vector<Terrain> terrains;
	std::vector<VulkanModel> terrainModels;
	std::vector<PointLight> pointLights;

	VulkanModel cube;
	VulkanModel waterPlane;
	VulkanTexture2D cubeTexture;
	VulkanTexture2D grassTexture;
	VulkanTexture2D waterTexture;
	Camera* camera;

	Skybox* skybox;

	//ImGUI *imGui = nullptr;

	VulkanDevice vulkanDevice;

	VulkanSwapChain vulkanSwapChain;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkPipeline waterPipeline;
	VkPipeline wireframePipeline;

	VkImage depthImage; 
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VulkanBuffer globalVariableBuffer;
	VulkanBuffer lightsInfoBuffer;
	VulkanBuffer cubeUniformBuffer;
	VulkanBuffer terrainUniformBuffer;
	VulkanBuffer waterUniformBuffer;


	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSetA;
	VkDescriptorPool descriptorPoolTerrain;
	std::vector<VkDescriptorSet> descriptorSetTerrain;
	VkDescriptorPool descriptorPoolWater;
	VkDescriptorSet descriptorSetWater;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
};

