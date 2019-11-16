#include "FrameGraph.h"

#include <map>

#include "Device.h"
#include "Initializers.h"
#include "RenderTools.h"
#include "SwapChain.h"
#include "Texture.h"

//// SUBPASS DESCRIPTION ////

void SubpassDescription::AddSubpassDependency (std::string subpass)
{
	subpass_dependencies.push_back (subpass);
}
void SubpassDescription::AddImageInput (std::string name) { input_attachments.push_back (name); }
void SubpassDescription::AddColorOutput (std::string name) { color_attachments.push_back (name); }
void SubpassDescription::AddResolveAttachments (std::string name)
{
	resolve_attachments.push_back (name);
}
void SubpassDescription::AddPreserveAttachments (std::string name)
{
	preserve_attachments.push_back (name);
}
void SubpassDescription::SetDepthStencil (std::string name, DepthStencilAccess access)
{
	depth_stencil_attachment = name;
	depth_stencil_access = access;
}

std::vector<std::string> SubpassDescription::AttachmentsUsed (AttachmentMap const& attachment_map) const
{
	std::vector<std::string> attachments;
	for (auto& item : input_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : color_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : resolve_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : preserve_attachments)
	{
		attachments.push_back (item);
	}
	if (depth_stencil_attachment.has_value ()) attachments.push_back (*depth_stencil_attachment);
	return attachments;
}

void SubpassDescription::AddClearColor (std::string attachment_name, VkClearValue color)
{
	clear_values[attachment_name] = color;
}

void SubpassDescription::SetFunction (RenderFunc&& func) { this->func = std::move (func); }

//// ATTACHMENT USE ////

AttachmentUse::AttachmentUse (RenderPassAttachment rpAttach, uint32_t index)
: format (rpAttach.format), rpAttach (rpAttach), index (index)
{
}

VkAttachmentDescription AttachmentUse::Get ()
{
	VkAttachmentDescription desc;
	desc.flags = 0; // doesn't alias
	desc.format = format;
	desc.samples = sampleCount;
	desc.loadOp = loadOp;
	desc.storeOp = storeOp;
	desc.stencilLoadOp = stencilLoadOp;
	desc.stencilStoreOp = stencilStoreOp;
	desc.initialLayout = initialLayout;
	desc.finalLayout = finalLayout;
	return desc;
}

//// RENDERPASS DESCRIPTION ////

void RenderPassDescription::AddSubpass (SubpassDescription subpass)
{
	subpasses.push_back (subpass);
}

VkRenderPassCreateInfo RenderPassDescription::GetRenderPassCreate (AttachmentMap& attachment_map)
{
	// Get all used attachments from subpasses(ignoring duplicate usages with std::unordered_set)
	std::unordered_set<std::string> used_attachment_names;
	for (auto& rp_subpass : subpasses)
	{
		auto sub_attaches = rp_subpass.AttachmentsUsed (attachment_map);
		for (auto& attach_name : sub_attaches)
		{
			used_attachment_names.insert (attach_name);
		}
	}

	// std::vector<AttachmentUse> attachment_uses;
	int index = 0;
	for (auto& attach_name : used_attachment_names)
	{
		if (attachment_map.count (attach_name) == 1)
		{
			attachment_uses.emplace_back (attachment_map.at (attach_name), index++);
		}
	}

	// map of attachment names and their index, for quick access - rather than a raw vector
	std::unordered_map<std::string, uint32_t> used_attachments;
	index = 0;
	for (auto& name : used_attachment_names)
		used_attachments[name] = index++;

	// create subpasses

	// std::vector<VulkanSubpassDescription> vulkan_sb_descriptions;
	for (auto& rp_subpass : subpasses)
	{
		VulkanSubpassDescription vulkan_desc;
		for (auto& name : rp_subpass.input_attachments)
			vulkan_desc.ar_inputs.push_back (
			    { (uint32_t)used_attachments.at (name), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

		for (auto& name : rp_subpass.color_attachments)
			vulkan_desc.ar_colors.push_back (
			    { (uint32_t)used_attachments.at (name), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		for (auto& name : rp_subpass.resolve_attachments)
			vulkan_desc.ar_resolves.push_back (
			    { (uint32_t)used_attachments.at (name), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		for (auto& name : rp_subpass.preserve_attachments)
			vulkan_desc.ar_preserves.push_back ((uint32_t)used_attachments.at (name));

		if (rp_subpass.depth_stencil_attachment.has_value ())
		{
			vulkan_desc.has_depth_stencil = true;
			vulkan_desc.ar_depth_stencil = { (uint32_t)used_attachments.at (
				                                 rp_subpass.depth_stencil_attachment.value ()),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		}

		vulkan_desc.desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		vulkan_sb_descriptions.push_back (vulkan_desc);
	}

	// get pointer arrays ready (ugh, c api...)
	// std::vector<VkSubpassDescription> sb_descriptions;
	for (auto& desc : vulkan_sb_descriptions)
		sb_descriptions.push_back (desc.Get ());


	// create subpass dependencies
	std::vector<SubpassDependency> vulkan_sb_dependencies;


	// std::vector<VkSubpassDependency> sb_dependencies;
	for (auto& desc : vulkan_sb_dependencies)
		sb_dependencies.push_back (desc.Get ());
	// TODO Subpass Dependencies


	// get attachment reference details
	for (auto& attach : attachment_uses)
	{
		bool is_input = false;
		bool is_output = false;

		bool is_depth_image = false;
		bool is_depth_input = false;
		bool is_depth_output = false;

		uint32_t subpass_earliest_use = 0;
		uint32_t subpass_latest_use = (int)vulkan_sb_descriptions.size () - 1;

		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		for (uint32_t i = 0; i < vulkan_sb_descriptions.size (); i++)
		{
			for (auto& ref : vulkan_sb_descriptions.at (i).ar_inputs)
			{
				if (ref.attachment == attach.index && subpass_earliest_use > ref.attachment)
				{
					subpass_earliest_use = i;
				}
				if (ref.attachment == attach.index && subpass_latest_use < ref.attachment)
				{
					subpass_latest_use = i;
				}
				is_input = true;
				initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			for (auto& ref : vulkan_sb_descriptions.at (i).ar_colors)
			{
				if (ref.attachment == attach.index && subpass_earliest_use > ref.attachment)
				{
					subpass_earliest_use = i;
				}
				if (ref.attachment == attach.index && subpass_latest_use < ref.attachment)
				{
					subpass_latest_use = i;
				}
				is_output = true;
				initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			for (auto& ref : vulkan_sb_descriptions.at (i).ar_resolves)
			{
				if (ref.attachment == attach.index && subpass_earliest_use > ref.attachment)
				{
					subpass_earliest_use = i;
				}
				if (ref.attachment == attach.index && subpass_latest_use < ref.attachment)
				{
					subpass_latest_use = i;
				}
			}
			for (auto& ref : vulkan_sb_descriptions.at (i).ar_preserves)
			{
				if (ref == attach.index && subpass_earliest_use > ref)
				{
					subpass_earliest_use = i;
				}
				if (ref == attach.index && subpass_latest_use < ref)
				{
					subpass_latest_use = i;
				}
			}
			if (vulkan_sb_descriptions.at (i).has_depth_stencil &&
			    vulkan_sb_descriptions.at (i).ar_depth_stencil.attachment == attach.index)
			{
				if (vulkan_sb_descriptions.at (i).ar_depth_stencil.attachment == attach.index &&
				    subpass_earliest_use > vulkan_sb_descriptions.at (i).ar_depth_stencil.attachment)
				{
					subpass_earliest_use = i;
				}
				if (vulkan_sb_descriptions.at (i).ar_depth_stencil.attachment == attach.index &&
				    subpass_latest_use < vulkan_sb_descriptions.at (i).ar_depth_stencil.attachment)
				{
					subpass_latest_use = i;
				}
				is_depth_image = true;
				is_depth_input = false;
				is_depth_output = true;
			}
		}

		attach.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		attach.loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		if (is_input)
		{
			attach.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		}
		else
		{
			attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		attach.storeOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		if (is_output)
		{
			attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else
		{
			attach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		if (is_depth_input)
		{
			attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		}
		else
		{
			attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		if (is_depth_output)
		{
			attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else
		{
			attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		// defaults
		if (attach.loadOp == VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
		{
			attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
		if (attach.storeOp == VK_ATTACHMENT_STORE_OP_MAX_ENUM)
		{
			attach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		if (attach.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
		{
			attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
		if (attach.stencilStoreOp == VK_ATTACHMENT_STORE_OP_MAX_ENUM)
		{
			attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		if (is_depth_image && is_depth_input)
		{
			initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		if (is_depth_image && is_depth_output)
		{
			initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		if (present_attachment && is_output && !is_depth_output)
		{
			initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		attach.initialLayout = initialLayout;
		attach.finalLayout = finalLayout;
	}

	for (auto& a : attachment_uses)
	{
		rp_attachments.push_back (a.Get ());
	}

	VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo ();
	renderPassInfo.attachmentCount = static_cast<uint32_t> (rp_attachments.size ());
	renderPassInfo.pAttachments = rp_attachments.data ();
	renderPassInfo.subpassCount = (uint32_t)sb_descriptions.size ();
	renderPassInfo.pSubpasses = sb_descriptions.data ();
	renderPassInfo.dependencyCount = (uint32_t)sb_dependencies.size ();
	renderPassInfo.pDependencies = sb_dependencies.data ();

	std::map<int, VkClearValue> map_clearColors;

	// get all the clear colors by name, put them in a map by their index
	for (auto& subpass : subpasses)
	{
		for (auto& [name, color] : subpass.clear_values)
		{
			map_clearColors[used_attachments.at (name)] = color;
		}
	}
	// traverse the map in order to get a vector of the clear colors
	for (auto& [index, color] : map_clearColors)
	{
		clear_values.push_back (color);
	}
	return renderPassInfo;
}

std::vector<RenderFunc> RenderPassDescription::GetSubpassFunctions ()
{
	std::vector<RenderFunc> funcs;

	for (auto& subpass : subpasses)
	{
		funcs.push_back (std::move (subpass.func));
	}
	return funcs;
}

std::vector<std::string> RenderPassDescription::GetUsedAttachmentNames ()
{
	std::vector<std::string> attachments;
	for (auto& attachment : attachment_uses)
	{
		attachments.push_back (attachment.rpAttach.name);
	}
	return attachments;
}


//// FRAME BUFFER ////

FrameBuffer::FrameBuffer (VulkanDevice& device,
    std::vector<VkImageView> image_views,
    VkRenderPass renderPass,
    uint32_t width,
    uint32_t height,
    uint32_t layers)
: device (device.device)
{
	VkFramebufferCreateInfo framebufferInfo = initializers::framebufferCreateInfo ();
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t> (image_views.size ());
	framebufferInfo.pAttachments = image_views.data ();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = layers;

	VK_CHECK_RESULT (vkCreateFramebuffer (device.device, &framebufferInfo, nullptr, &framebuffer));
}

FrameBuffer::~FrameBuffer ()
{
	if (framebuffer != nullptr) vkDestroyFramebuffer (device, framebuffer, nullptr);
}

FrameBuffer::FrameBuffer (FrameBuffer&& fb) : device (fb.device), framebuffer (fb.framebuffer)
{
	fb.framebuffer = nullptr;
}
FrameBuffer& FrameBuffer::operator= (FrameBuffer&& fb)
{
	device = fb.device;
	framebuffer = fb.framebuffer;
	fb.framebuffer = nullptr;
	return *this;
}

//// RENDER PASS ////

RenderPass::RenderPass (VkDevice device, RenderPassDescription desc, AttachmentMap& attachments)
: device (device), desc (desc)
{

	auto renderPassInfo = desc.GetRenderPassCreate (attachments);

	if (vkCreateRenderPass (device, &renderPassInfo, nullptr, &rp) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create render pass!");
	}

	subpassFuncs = std::move (desc.GetSubpassFunctions ());
}

RenderPass::~RenderPass ()
{
	if (rp != nullptr) vkDestroyRenderPass (device, rp, nullptr);
}

RenderPass::RenderPass (RenderPass&& rp)
: device (rp.device), subpassFuncs (std::move (rp.subpassFuncs)), desc (rp.desc), rp (rp.rp)
{
	rp.rp = nullptr;
}
RenderPass& RenderPass::operator= (RenderPass&& rp)
{
	device = rp.device;
	subpassFuncs = std::move (rp.subpassFuncs);
	this->rp = rp.rp;
	desc = rp.desc;
	rp.rp = nullptr;
	return *this;
}

void RenderPass::BuildCmdBuf (VkCommandBuffer cmdBuf, FrameBufferView fb_view)
{
	VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo (
	    rp, fb_view.fb, fb_view.view.offset, fb_view.view.extent, desc.clear_values);


	vkCmdBeginRenderPass (cmdBuf, &renderPassInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	if (subpassFuncs.size () == 0)
	{
		// nothing to draw;
	}
	else if (subpassFuncs.size () == 1)
	{
		subpassFuncs.at (0) (cmdBuf);
	}
	else
	{
		for (auto& func : subpassFuncs)
		{
			func (cmdBuf);
			vkCmdNextSubpass (cmdBuf, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
		}
	}
	vkCmdEndRenderPass (cmdBuf);
}

//// FRAME GRAPH BUILDER ////


void FrameGraphBuilder::AddAttachment (RenderPassAttachment attachment)
{

	attachments[attachment.name] = attachment;
}

void FrameGraphBuilder::AddRenderPass (RenderPassDescription renderPass)
{
	render_passes[renderPass.name] = renderPass;
}

void FrameGraphBuilder::SetFinalRenderPassName (std::string name) { final_renderpass = name; }

void FrameGraphBuilder::SetFinalOutputAttachmentName (std::string name)
{
	final_output_attachment = name;
}


//// FRAME GRAPH ////

FrameGraph::FrameGraph (VulkanDevice& device, VulkanSwapChain& swapchain, FrameGraphBuilder builder)
: device (device), swapchain (swapchain), builder (builder)
{
	int swapchain_count = swapchain.GetChainCount ();

	auto& final_attachment = builder.attachments.at (builder.final_output_attachment);
	auto& final_renderpass_desc = builder.render_passes.at (builder.final_renderpass);
	final_renderpass_desc.present_attachment = true;
	// for (auto&[name, a] : builder.attachments) {
	//	for(auto& sub : final_renderpass.subpasses){
	//		for (auto& a : sub.color_attachments) {
	//			if (name == a) {
	//				builder.attachments.at(name).format = VK_FORMAT_R8G8B8A8_SINT;
	//			}
	//		}
	//	}
	//}

	for (auto& [name, attachment] : builder.attachments)
	{
		if (name != builder.final_output_attachment) // swapchain owns presentation resources
		{
			uint32_t width = attachment.width != 0 ? attachment.width : swapchain.GetImageExtent ().width;
			uint32_t height =
			    attachment.height != 0 ? attachment.height : swapchain.GetImageExtent ().height;

			TexCreateDetails tex_details (
			    attachment.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0, width, height);
			render_targets[name] = std::make_unique<VulkanTexture> (device, tex_details);
		}
	}

	for (auto& [name, pass] : builder.render_passes)
	{
		if (name == builder.final_renderpass)
		{
			final_renderpass = std::make_unique<RenderPass> (device.device, pass, builder.attachments);
		}
		else
		{
			RenderPass render_pass = RenderPass (device.device, pass, builder.attachments);
			auto attachments = render_pass.GetUsedAttachmentNames ();
			std::vector<VkImageView> views;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t layers = 1;
			for (auto& attachment : attachments)
			{
				auto tex = render_targets.at (attachment).get ();
				views.push_back (tex->imageView);
				uint32_t width = tex->GetWidth ();
				uint32_t height = tex->GetHeight ();
				uint32_t layers = tex->GetLayers ();
			}
			framebuffers.emplace (name,
			    std::move (FrameBuffer (device, views, render_pass.Get (), width, height, layers)));
			render_passes.push_back (std::move (render_pass));
		}
	}
	CreatePresentResources ();
}

FrameGraph::~FrameGraph () {}

VkRenderPass FrameGraph::Get (int index) const { return render_passes.at (index).Get (); }


void FrameGraph::FillCommandBuffer (VkCommandBuffer cmdBuf, FrameBufferView frame_buffer_view)
{
	// render_passes.at (render_pass_index).BuildCmdBuf (cmdBuf, frame_buffer_view);
}

void FrameGraph::RecreatePresentResources ()
{
	swapchain_framebuffers.clear ();
	CreatePresentResources ();
}

void FrameGraph::CreatePresentResources ()
{
	int swapchain_count = swapchain.GetChainCount ();

	for (int i = 0; i < swapchain_count; i++)
	{
		auto attachments = final_renderpass->GetUsedAttachmentNames ();
		std::vector<VkImageView> views;
		for (auto& attachment : attachments)
		{
			views.push_back (render_targets.at (attachment)->imageView);
		}

		views.push_back (swapchain.GetSwapChainImageView (i));
		auto extent = swapchain.GetImageExtent ();
		swapchain_framebuffers.push_back (std::move (
		    FrameBuffer (device, views, final_renderpass->Get (), extent.width, extent.height, 1)));
	}
}