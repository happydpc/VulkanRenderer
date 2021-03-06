#include "SkyboxRenderer.h"

#include <utility>

#include "rendering/backend/BackEnd.h"

struct SkyboxUniformBuffer
{
	cml::mat4f proj;
	cml::mat4f view;
};

Skybox::Skybox (RenderCameras& render_cameras,
    Models& models,
    DescriptorLayout descriptor_layout,
    DescriptorPool& descriptor_pool,
    DescriptorSet descriptor_set,
    VulkanBuffer& uniform_buffer,
    ModelID skybox_cube_model,
    PipelineLayout& pipe_layout,
    GraphicsPipeline& pipe)
: render_cameras (render_cameras),
  models (models),
  descriptor_layout (std::move (descriptor_layout)),
  descriptor_pool (std::move (descriptor_pool)),
  descriptor_set (descriptor_set),
  uniform_buffer (std::move (uniform_buffer)),
  skybox_cube_model (skybox_cube_model),
  pipe_layout (std::move (pipe_layout)),
  pipe (std::move (pipe))
{
}

void Skybox::update (ViewCameraID cam_id)
{
	ViewCameraData& cam = render_cameras.get_camera_data (cam_id);

	SkyboxUniformBuffer sbo = {};
	sbo.proj = cam.get_proj_mat ();
	sbo.view = cam.get_view_mat ();
	sbo.view.set_col (3, cml::vec4f::w_positive);

	uniform_buffer.copy_to_buffer (sbo);
};


void Skybox::Draw (VkCommandBuffer commandBuffer)
{
	descriptor_set.bind (commandBuffer, pipe_layout.get (), 2);
	pipe.bind (commandBuffer);
	models.draw_indexed (commandBuffer, skybox_cube_model);
}

std::optional<Skybox> CreateSkybox (
    BackEnd& back_end, RenderCameras& render_cameras, Resource::Texture::TexID cube_map, VkRenderPass render_pass, uint32_t subpass)
{

	std::vector<DescriptorSetLayoutBinding> m_bindings = {
		{ DescriptorType::uniform_buffer, ShaderStage::vertex, 0, 1 },
		{ DescriptorType::combined_image_sampler, ShaderStage::fragment, 1, 1 }
	};

	auto descriptor_layout = DescriptorLayout (back_end.device.device, m_bindings);
	auto descriptor_pool = DescriptorPool (back_end.device.device, descriptor_layout.get (), m_bindings, 2);
	auto descriptor_set = descriptor_pool.allocate ();
	auto skybox_cube_model = back_end.models.create_model (Resource::Mesh::create_cube ());

	auto uniform_buffer = VulkanBuffer (back_end.device, uniform_details (sizeof (SkyboxUniformBuffer)));

	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 3);
	auto vulkan_cube_map = back_end.textures.create_cube_map (cube_map, details);


	std::vector<DescriptorUse> writes = {
		{ 0, 1, uniform_buffer.get_descriptor_type (), { uniform_buffer.get_descriptor_info () } },
		{ 1, 1, back_end.textures.get_descriptor_type (vulkan_cube_map), { back_end.textures.get_resource (vulkan_cube_map) } }
	};
	descriptor_set.update (back_end.device.device, writes);

	auto vert = back_end.shaders.GetModule ("skybox.vert", ShaderType::vertex);
	auto frag = back_end.shaders.GetModule ("skybox.frag", ShaderType::fragment);

	PipelineBuilder builder{ back_end.device.device, back_end.pipeline_cache.get () };
	builder.SetShaderModuleSet (ShaderModuleSet (vert.value (), frag.value ()))
	    .UseModelVertexLayout (back_end.models.get_layout (skybox_cube_model))
	    .AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f)
	    .AddScissor (1, 1, 0, 0)
	    .SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false)
	    .SetRasterizer (
	        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE)
	    .SetMultisampling (VK_SAMPLE_COUNT_1_BIT)
	    .set_depth_stencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE)
	    .AddColorBlendingAttachment (VK_FALSE,
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_BLEND_OP_ADD,
	        VK_BLEND_FACTOR_SRC_COLOR,
	        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	        VK_BLEND_OP_ADD,
	        VK_BLEND_FACTOR_ONE,
	        VK_BLEND_FACTOR_ZERO)
	    // outline.AddDescriptorLayouts (double_buffer_man.GetGlobalLayouts ());
	    .AddDescriptorLayout (descriptor_layout.get ())
	    .AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	auto pipe_layout = builder.CreateLayout ();
	if (!pipe_layout) return {};
	auto pipe = builder.CreatePipeline (pipe_layout.value (), render_pass, subpass);
	if (!pipe) return {};

	return Skybox{ render_cameras,
		back_end.models,
		std::move (descriptor_layout),
		descriptor_pool,
		descriptor_set,
		uniform_buffer,
		skybox_cube_model,
		pipe_layout.value (),
		pipe.value () };
}