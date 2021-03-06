//#include "Water.h"
//
//#include "cml/cml.h"
//
// Water::Water (Resource::Resources& resourceMan, VulkanRenderer& renderer) : renderer (renderer)
//{
//	model = std::make_unique<VulkanModel> (
//	    renderer.device, renderer.async_task_queue, create_water_plane_subdiv (13, 40));
//
//	texture = resourceMan.textures.get_tex_id_by_name ("water_normal");
//	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8);
//	vulkanTexture = renderer.textures.create_texture_2d (texture, details);
//
//	uniformBuffer =
//	    std::make_unique<VulkanBuffer> (renderer.device, uniform_details (sizeof (ModelBufferObject)));
//
//	ubo.model = cml::mat4f ();
//	ubo.model = ubo.model.translate (cml::vec3f (0, 0, 0));
//	ubo.normal = cml::to_mat4 (cml::to_mat3 (ubo.model).inverse ().transpose ());
//
//	uniformBuffer->copy_to_buffer (ubo);
//
//	descriptor = std::make_unique<VulkanDescriptor> (renderer.device);
//
//	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
//	m_bindings.push_back (VulkanDescriptor::CreateBinding (
//	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
//	m_bindings.push_back (VulkanDescriptor::CreateBinding (
//	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
//	// m_bindings.push_back (VulkanDescriptor::CreateBinding (
//	//    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
//	descriptor->SetupLayout (m_bindings);
//
//	std::vector<DescriptorPoolSize> poolSizes;
//	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
//	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
//	// poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
//	descriptor->SetupPool (poolSizes);
//
//	m_descriptorSet = descriptor->CreateDescriptorSet ();
//
//	std::vector<DescriptorUse> writes;
//	writes.push_back (DescriptorUse (0, 1, uniformBuffer->get_resource ()));
//	writes.push_back (DescriptorUse (1, 1, renderer.textures.get_resource (vulkanTexture)));
//	descriptor->UpdateDescriptorSet (m_descriptorSet, writes);
//
//	PipelineOutline out;
//
//	auto water_vert = renderer.shaders.get_module ("water", ShaderType::vertex);
//	auto water_frag = renderer.shaders.get_module ("water", ShaderType::fragment);
//
//	ShaderModuleSet water_shaders;
//	water_shaders.Vertex (water_vert.value ()).Fragment (water_frag.value ());
//	out.SetShaderModuleSet (water_shaders);
//
//	out.UseModelVertexLayout (*model.get ());
//
//	out.SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
//
//	out.AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
//	out.AddScissor (1, 1, 0, 0);
//
//	out.SetRasterizer (
//	    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
//
//	out.SetMultisampling (VK_SAMPLE_COUNT_1_BIT);
//	out.set_depth_stencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
//	out.AddColorBlendingAttachment (VK_TRUE,
//	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
//	    VK_BLEND_OP_ADD,
//	    VK_BLEND_FACTOR_SRC_COLOR,
//	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
//	    VK_BLEND_OP_ADD,
//	    VK_BLEND_FACTOR_ONE,
//	    VK_BLEND_FACTOR_ZERO);
//
//	out.AddDescriptorLayouts (renderer.dynamic_data.GetGlobalLayouts ());
//	out.AddDescriptorLayout (descriptor->get_layout ());
//
//	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
//
//	pipe = renderer.pipelines.MakePipe (out, renderer.GetRelevantRenderpass (RenderableType::opaque));
//
//	out.SetRasterizer (
//	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
//
//	wireframe =
//	    renderer.pipelines.MakePipe (out, renderer.GetRelevantRenderpass (RenderableType::opaque));
//}
//
// void Water::UpdateUniform (cml::vec3f camera_pos)
//{
//	auto pos = camera_pos;
//	pos.y = 0;
//	ubo.model = cml::mat4f (1.0f).translate (pos);
//	uniformBuffer->copy_to_buffer (ubo);
//}
//
// void Water::Draw (VkCommandBuffer cmdBuf, bool wireframe)
//{
//	model->BindModel (cmdBuf);
//	if (wireframe)
//		renderer.pipelines.BindPipe (wireframe, cmdBuf);
//	else
//		renderer.pipelines.BindPipe (pipe, cmdBuf);
//
//	vkCmdBindDescriptorSets (cmdBuf,
//	    VK_PIPELINE_BIND_POINT_GRAPHICS,
//	    renderer.pipelines.GetPipeLayout (pipe),
//	    2,
//	    1,
//	    &m_descriptorSet.set,
//	    0,
//	    nullptr);
//
//	vkCmdDrawIndexed (cmdBuf, model->indexCount, 1, 0, 0, 0);
//}
