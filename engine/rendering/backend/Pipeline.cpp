#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "resources/Mesh.h"

#include "Model.h"
#include "rendering/Initializers.h"

//// PipelineLayout ////

auto create_pipeline_layout (VkDevice device,
    std::vector<VkDescriptorSetLayout> desc_set_layouts,
    std::vector<VkPushConstantRange> push_constant_ranges)
{
	auto layoutInfo = initializers::pipeline_set_layout_create_info (desc_set_layouts, push_constant_ranges);
	VkPipelineLayout layout;
	VK_CHECK_RESULT (vkCreatePipelineLayout (device, &layoutInfo, nullptr, &layout));
	return VulkanHandle (device, layout, vkDestroyPipelineLayout);
}

PipelineLayout::PipelineLayout (VkDevice device, VkPipelineLayout layout)
: layout (VulkanHandle (device, layout, vkDestroyPipelineLayout))
{
}

PipelineLayout::PipelineLayout (VkDevice device,
    std::vector<VkDescriptorSetLayout> desc_set_layouts,
    std::vector<VkPushConstantRange> push_constant_ranges)
: layout (create_pipeline_layout (device, desc_set_layouts, push_constant_ranges))
{
}

//// Graphics Pipeline ////

GraphicsPipeline::GraphicsPipeline (VkDevice device, VkPipeline pipeline)
: pipeline (device, pipeline, vkDestroyPipeline)
{
}

void GraphicsPipeline::bind (VkCommandBuffer cmdBuf)
{
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
}

//// Compute Pipeline ////

ComputePipeline::ComputePipeline (VkDevice device, VkPipeline pipeline)
: pipeline (device, pipeline, vkDestroyPipeline)
{
}

void ComputePipeline::bind (VkCommandBuffer cmdBuf)
{
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle);
}


//// Pipeline Builder ////

PipelineBuilder::PipelineBuilder (VkDevice device, VkPipelineCache cache)
: device (device), cache (cache)
{
}

std::optional<PipelineLayout> PipelineBuilder::CreateLayout () const
{
	auto layoutInfo = initializers::pipeline_set_layout_create_info (layouts, pushConstantRanges);
	VkPipelineLayout layout;
	VkResult res = vkCreatePipelineLayout (device, &layoutInfo, nullptr, &layout);
	if (res != VK_SUCCESS) return {};
	return PipelineLayout{ device, layout };
}

std::optional<GraphicsPipeline> PipelineBuilder::CreatePipeline (
    PipelineLayout const& layout, VkRenderPass render_pass, uint32_t subpass) const
{
	auto info = initializers::graphics_pipeline_create_info (layout.get (), render_pass, subpass, 0);
	info.basePipelineHandle = VK_NULL_HANDLE;

	auto shaderStages = set.ShaderStageCreateInfos ();
	info.stageCount = (uint32_t)shaderStages.size ();
	info.pStages = shaderStages.data ();

	info.pInputAssemblyState = &inputAssembly;

	auto vertexInput = initializers::pipeline_vertex_input_state_create_info (
	    vertexInputBindingDescription, vertexInputAttributeDescriptions);

	info.pVertexInputState = &vertexInput;

	auto viewportInfo = initializers::pipeline_viewport_state_create_info (viewports, scissors, 0);
	info.pViewportState = &viewportInfo;

	info.pRasterizationState = &rasterizer;
	info.pMultisampleState = &multisampling;
	info.pDepthStencilState = &depthStencil;

	auto colorBlending = initializers::pipeline_color_blend_state_create_info (colorBlendAttachments);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	info.pColorBlendState = &colorBlending;

	VkPipelineDynamicStateCreateInfo dynamicInfo =
	    initializers::pipeline_dynamic_state_create_info (dynamicStates, 0);
	info.pDynamicState = &dynamicInfo;

	VkPipeline pipe;
	VkResult res = vkCreateGraphicsPipelines (device, cache, 1, &info, nullptr, &pipe);
	if (res != VK_SUCCESS)
	{
		return {};
	}
	return GraphicsPipeline{ device, pipe };
}

PipelineBuilder& PipelineBuilder::SetShaderModuleSet (ShaderModuleSet set)
{
	this->set = set;
	return *this;
}
PipelineBuilder& PipelineBuilder::UseModelVertexLayout (VertexLayout const& layout)
{
	for (auto& desc : layout.bindingDesc)
		vertexInputBindingDescription.push_back (desc);
	for (auto& desc : layout.attribDesc)
		vertexInputAttributeDescriptions.push_back (desc);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddVertexLayout (
    VkVertexInputBindingDescription bind, VkVertexInputAttributeDescription attrib)
{
	vertexInputBindingDescription.push_back (bind);
	vertexInputAttributeDescriptions.push_back (attrib);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddVertexLayouts (std::vector<VkVertexInputBindingDescription> binds,
    std::vector<VkVertexInputAttributeDescription> attribs)
{
	vertexInputBindingDescription.insert (
	    std::end (vertexInputBindingDescription), std::begin (binds), std::end (binds));
	vertexInputAttributeDescriptions.insert (
	    std::end (vertexInputAttributeDescriptions), std::begin (attribs), std::end (attribs));
	return *this;
}
PipelineBuilder& PipelineBuilder::SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart)
{
	inputAssembly = initializers::pipeline_input_assembly_state_create_info (topology, primitiveRestart);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddDescriptorLayouts (std::vector<VkDescriptorSetLayout> layouts)
{
	this->layouts.insert (std::end (this->layouts), std::begin (layouts), std::end (layouts));
	return *this;
}
PipelineBuilder& PipelineBuilder::AddDescriptorLayout (VkDescriptorSetLayout layout)
{
	layouts.push_back (layout);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddViewport (VkViewport viewport)
{
	viewports.push_back (viewport);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddViewport (
    float width, float height, float minDepth, float maxDepth, float x, float y)
{
	auto viewport = initializers::viewport (width, height, minDepth, maxDepth);
	viewport.x = x;
	viewport.y = y;
	viewports.push_back (viewport);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddScissor (VkRect2D scissor)
{
	scissors.push_back (scissor);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddScissor (uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
{
	scissors.push_back (initializers::rect2D (width, height, offsetX, offsetY));
	return *this;
}
PipelineBuilder& PipelineBuilder::SetRasterizer (VkPolygonMode polygonMode,
    VkCullModeFlagBits cullModeFlagBits,
    VkFrontFace frontFace,
    VkBool32 depthClampEnable,
    VkBool32 rasterizerDiscardEnable,
    float lineWidth,
    VkBool32 depthBiasEnable)
{
	rasterizer = initializers::pipeline_rasterization_state_create_info (polygonMode, cullModeFlagBits, frontFace);
	rasterizer.depthClampEnable = depthClampEnable;
	rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	rasterizer.lineWidth = lineWidth;
	rasterizer.depthBiasEnable = depthBiasEnable;
	return *this;
}
// No Multisampling support right now
PipelineBuilder& PipelineBuilder::SetMultisampling (VkSampleCountFlagBits sampleCountFlags)
{
	multisampling = initializers::pipeline_multisample_state_create_info (sampleCountFlags);
	multisampling.sampleShadingEnable = VK_FALSE;
	return *this;
}
PipelineBuilder& PipelineBuilder::set_depth_stencil (VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkBool32 depthBoundsTestEnable,
    VkBool32 stencilTestEnable)
{
	depthStencil = initializers::pipeline_depth_stencil_state_create_info (
	    depthTestEnable, depthWriteEnable, depthCompareOp);
	depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	depthStencil.stencilTestEnable = stencilTestEnable;
	return *this;
}
PipelineBuilder& PipelineBuilder::AddColorBlendingAttachment (VkBool32 blendEnable,
    VkColorComponentFlags colorWriteMask,
    VkBlendOp colorBlendOp,
    VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor,
    VkBlendOp alphaBlendOp,
    VkBlendFactor srcAlphaBlendFactor,
    VkBlendFactor dstAlphaBlendFactor)
{
	auto colorBlendAttachment = initializers::pipelineColorBlendAttachmentState (colorWriteMask, blendEnable);
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
	colorBlendAttachments.push_back (colorBlendAttachment);
	return *this;
}

PipelineBuilder& PipelineBuilder::AddColorBlendingAttachment (VkPipelineColorBlendAttachmentState attachment)
{
	colorBlendAttachments.push_back (attachment);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddDynamicStates (std::vector<VkDynamicState> states)
{
	dynamicStates.insert (std::end (dynamicStates), std::begin (states), std::end (states));
	return *this;
}
PipelineBuilder& PipelineBuilder::AddDynamicState (VkDynamicState state)
{
	dynamicStates.push_back (state);
	return *this;
}
PipelineBuilder& PipelineBuilder::AddPushConstantRange (VkPushConstantRange pushConstantRange)
{
	pushConstantRanges.push_back (pushConstantRange);
	return *this;
}


std::optional<ComputePipeline> BuildComputePipeline (
    VkDevice device, PipelineLayout const& layout, ShaderModule shader_module, VkPipelineCache cache)
{
	auto info = initializers::compute_pipeline_create_info (layout.get (), 0);
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.stage = initializers::pipeline_shader_stage_create_info (
	    VK_SHADER_STAGE_COMPUTE_BIT, shader_module.get ());

	VkPipeline pipe;
	VkResult res = vkCreateComputePipelines (device, cache, 1, &info, nullptr, &pipe);
	if (res != VK_SUCCESS)
	{
		return {};
	}
	return ComputePipeline{ device, pipe };
}

//// PipelineCache ////

PipelineCache::PipelineCache (VkDevice device) : device (device)
{
	std::vector<std::byte> cache_data;

	if (std::filesystem::exists (".cache/pipeline_cache"))
	{
		std::ifstream in (".cache/pipeline_cache", std::ios::binary | std::ios::ate);
		auto size = in.tellg ();
		std::vector<std::byte> cache_data (size); // construct string to stream size
		in.seekg (0);
		in.read (reinterpret_cast<char*> (cache_data.data ()), size);
	}

	VkPipelineCacheCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	info.initialDataSize = cache_data.size ();
	info.pInitialData = cache_data.data ();

	vkCreatePipelineCache (device, &info, nullptr, &cache);
}

PipelineCache::~PipelineCache ()
{
	std::ofstream out (".cache/pipeline_cache", std::ios::binary | std::ios::out);
	size_t cache_size = 0;
	std::vector<std::byte> cache_data;
	vkGetPipelineCacheData (device, cache, &cache_size, nullptr);
	cache_data.resize (cache_size);
	vkGetPipelineCacheData (device, cache, &cache_size, cache_data.data ());

	out.write (reinterpret_cast<char*> (cache_data.data ()), cache_data.size ());

	vkDestroyPipelineCache (device, cache, nullptr);
}

VkPipelineCache PipelineCache::get () const { return cache; }