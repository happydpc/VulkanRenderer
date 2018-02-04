#include "Skybox.h"

Skybox::Skybox() {

};

Skybox::~Skybox() {

};

void Skybox::CleanUp() {
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

	model.destroy(renderer->device);
	vulkanCubeMap.destroy(renderer->device);

	//skyboxUniformBuffer.cleanBuffer();
}

void Skybox::InitSkybox(std::shared_ptr<VulkanRenderer> renderer) {
	this->renderer = renderer;

	//SetupUniformBuffer();
	SetupCubeMapImage();
	SetupDescriptor();
	SetupPipeline();

}

void Skybox::SetupUniformBuffer() {
	//renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
	//	(VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	//	&skyboxUniformBuffer, sizeof(SkyboxUniformBuffer));
}

void Skybox::SetupCubeMapImage() {
	vulkanCubeMap.loadFromTexture(renderer->device, skyboxCubeMap, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue);

}

void Skybox::SetupDescriptor() {
	descriptor = renderer->GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool(poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(2, 1, vulkanCubeMap.resource));
	descriptor->UpdateDescriptorSet(m_descriptorSet, writes);
}

void Skybox::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/skybox.vert.spv"));
	pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/skybox.frag.spv"));
	pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width, (float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 
		VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(mvp, VK_FALSE,  
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
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(mvp);
	
}

void Skybox::UpdateUniform(glm::mat4 proj, glm::mat4 view) {
	//SkyboxUniformBuffer sbo = {};
	//sbo.proj = proj;
	//sbo.view = glm::mat4(glm::mat3(view));
	//
	//skyboxUniformBuffer.map(renderer->device.device);
	//skyboxUniformBuffer.copyTo(&sbo, sizeof(sbo));
	//skyboxUniformBuffer.unmap();
};

//VkCommandBuffer Skybox::BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, 
//		VkCommandBufferInheritanceInfo inheritanceInfo) {
//
//	VkCommandBufferBeginInfo commandBufferBeginInfo = initializers::commandBufferBeginInfo();
//	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
//	commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
//
//	VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffer, &commandBufferBeginInfo));
//
//	
//	vkCmdBindDescriptorSets(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 0, 1, &descriptorSet, 0, nullptr);
//	vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(0));
//
//	VkDeviceSize offsets[1] = { 0 };
//	vkCmdBindVertexBuffers(secondaryCommandBuffer, 0, 1, &model.vertices.buffer, offsets);
//	vkCmdBindIndexBuffer(secondaryCommandBuffer, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
//	vkCmdDrawIndexed(secondaryCommandBuffer, static_cast<uint32_t>(model.indexCount), 1, 0, 0, 0);
//
//	VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffer));
//
//	return secondaryCommandBuffer;
//}


