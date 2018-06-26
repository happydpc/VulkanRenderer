#include "Water.h"

#include <glm/gtc/matrix_transform.hpp>

Water::Water(int numCells, float posX, float posY, float sizeX, float sizeY) : numCells(numCells), pos(glm::vec3(posX, 0, posY)), size(glm::vec3(sizeX, 0, sizeY))
{
	//WaterMesh = createFlatPlane(numCells, size);
}

Water::~Water() {
	CleanUp();
}


void Water::InitWater(VulkanRenderer* renderer, VulkanTexture2D& WaterVulkanTexture)
{
	this->renderer = renderer;

	SetupUniformBuffer();
	//SetupImage();
	//SetupModel();
	SetupDescriptor(WaterVulkanTexture);
	SetupPipeline();

	modelUniformBuffer = std::make_shared<VulkanBufferUniform>(renderer->device);
}

void Water::CleanUp()
{
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

	//WaterModel.destroy(renderer->device);
	//WaterVulkanTexture.destroy(renderer->device);

	modelUniformBuffer->CleanBuffer();
}

void Water::SetupUniformBuffer()
{
	//renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &modelUniformBuffer, sizeof(ModelBufferObject));
	modelUniformBuffer->CreateUniformBuffer(sizeof(ModelBufferObject));

	ModelBufferObject ubo = {};
	ubo.model = glm::translate(glm::mat4(), pos);
	ubo.normal = glm::transpose(glm::inverse(ubo.model));

	modelUniformBuffer->CopyToBuffer(&ubo, sizeof(ModelBufferObject));
}

void Water::SetupImage()
{
	//WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);
}

void Water::SetupModel()
{
	//WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);
}

void Water::SetupDescriptor(VulkanTexture2D& WaterVulkanTexture)
{
	descriptor = renderer->GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool(poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(0, 1, modelUniformBuffer->resource));
	writes.push_back(DescriptorUse(1, 1, WaterVulkanTexture.resource));
	descriptor->UpdateDescriptorSet(m_descriptorSet, writes);
}

void Water::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	//pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/water.vert.spv"));
	//pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	auto vert = renderer->shaderManager.loadShaderModule("assets/shaders/water.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer->shaderManager.loadShaderModule("assets/shaders/water.frag.spv", ShaderModuleType::fragment);

	ShaderModuleSet set(vert, frag, {}, {}, {});
	pipeMan.SetShaderModuleSet(mvp, set);
	
	
	pipeMan.SetVertexInput(mvp, Vertex_PosNormTexColor::getBindingDescription(), Vertex_PosNormTexColor::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width, (float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 
		VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(mvp, VK_TRUE, 
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, 
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending(mvp, 1, &mvp->pco.colorBlendAttachment); 
	
	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState(mvp, dynamicStateEnables);

	std::vector<VkDescriptorSetLayout> layouts;
	renderer->AddGlobalLayouts(layouts);
	layouts.push_back(descriptor->GetLayout());
	pipeMan.SetDescriptorSetLayout(mvp, layouts);

	pipeMan.BuildPipelineLayout(mvp);
	pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);

	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);

	//pipeMan.CleanShaderResources(mvp);
	//pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/Seascape.vert.spv"));
	//pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/Seascape.frag.spv"));
	
	//pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);
		
	//pipeMan.CleanShaderResources(mvp);
	
}

//void Water::BuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass)
//{
//	commandBuffers.resize(swapChain->swapChainFramebuffers.size());
//
//	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(renderer->device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());
//
//	if (vkAllocateCommandBuffers(renderer->device.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
//		throw std::runtime_error("failed to allocate command buffers!");
//	}
//
//	for (size_t i = 0; i < commandBuffers.size(); i++) {
//		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
//		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
//
//		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
//
//		VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
//		renderPassInfo.renderPass = *renderPass;
//		renderPassInfo.framebuffer = swapChain->swapChainFramebuffers[i];
//		renderPassInfo.renderArea.offset = { 0, 0 };
//		renderPassInfo.renderArea.extent = swapChain->swapChainExtent;
//
//		std::array<VkClearValue, 2> clearValues = {};
//		clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
//		clearValues[1].depthStencil = { 1.0f, 0 };
//
//		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
//		renderPassInfo.pClearValues = clearValues.data();
//
//		VkDeviceSize offsets[] = { 0 };
//
//		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
//		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 0, 1, &descriptorSet, 0, nullptr);
//
//		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &WaterModel.vmaBufferVertex, offsets);
//		vkCmdBindIndexBuffer(commandBuffers[i], WaterModel.vmaBufferIndex, 0, VK_INDEX_TYPE_UINT32);
//
//		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(WaterModel.indexCount), 1, 0, 0, 0);
//
//		vkCmdEndRenderPass(commandBuffers[i]);
//
//		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
//			throw std::runtime_error("failed to record command buffer!");
//		}
//	}
//}

//void Water::RebuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass)
//{
//	vkFreeCommandBuffers(renderer->device.device, renderer->device.graphics_queue_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
//
//	BuildCommandBuffer(swapChain, renderPass);
//}

//void Water::UpdateUniformBuffer(float time, glm::mat4 view)
//{
//	ModelBufferObject ubo = {};
//	ubo.model = glm::translate(glm::mat4(), pos);
//	ubo.normal = glm::transpose(glm::inverse(glm::mat3(glm::mat4())));
//
//	modelUniformBuffer.map(renderer->device.device);
//	modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
//	modelUniformBuffer.unmap();
//}