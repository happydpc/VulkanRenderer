#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

class VulkanDevice;

enum class DescriptorType
{
	sampler,
	combined_image_sampler,
	sampled_image,
	storage_image,
	uniform_texel_buffer,
	storage_texel_buffer,
	uniform_buffer,
	storage_buffer,
	uniform_buffer_dynamic,
	storage_buffer_dynamic,
	input_attachment
};

struct DescriptorSetLayoutBinding
{
	DescriptorType type;
	VkShaderStageFlagBits stage;
	uint32_t bind_point;
	uint32_t count;
};

namespace std
{
template <> struct hash<DescriptorSetLayoutBinding>
{
	std::size_t operator() (DescriptorSetLayoutBinding const& s) const noexcept
	{
		std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.type));
		std::size_t h2 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.stage));
		std::size_t h3 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.bind_point));
		std::size_t h4 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.count));
		return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
	}
};
} // namespace std
struct DescriptorLayout
{
	DescriptorLayout (std::vector<DescriptorSetLayoutBinding> bindings) : bindings (bindings) {}
	const std::vector<DescriptorSetLayoutBinding> bindings;
};

std::vector<VkDescriptorSetLayoutBinding> GetVkDescriptorLayout (DescriptorLayout layout);

namespace std
{
template <> struct hash<DescriptorLayout>
{
	std::size_t operator() (DescriptorLayout const& s) const noexcept
	{
		int i = 0;
		std::size_t out;
		for (auto& binding : s.bindings)
		{
			out ^= (std::hash<DescriptorSetLayoutBinding>{}(binding) << i++);
		}
		return out;
	}
};
} // namespace std

class DescriptorResource
{
	public:
	std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> info;
	VkDescriptorType type = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;

	DescriptorResource (VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	DescriptorResource (VkDescriptorType type, VkSampler sampler, VkImageView imageView, VkImageLayout layout);
};

class DescriptorUse
{
	public:
	DescriptorUse (uint32_t bindPoint, uint32_t count, DescriptorResource resource);

	VkWriteDescriptorSet GetWriteDescriptorSet (VkDescriptorSet set);

	private:
	uint32_t bindPoint;
	uint32_t count;

	DescriptorResource resource;
};
using LayoutID = uint16_t; // also indexes for the pool
using PoolID = uint16_t;   // which vkPool to index in the pool
class DescriptorSet
{
	public:
	DescriptorSet (VkDescriptorSet set, LayoutID layout_id, PoolID pool_id);

	void Update (VkDevice device, std::vector<DescriptorUse> descriptors) const;

	void Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout) const;

	VkDescriptorSet const& GetSet () const { return set; }
	LayoutID GetLayoutID () const { return layout_id; }
	PoolID GetPoolID () const { return pool_id; }

	private:
	VkDescriptorSet set;
	LayoutID layout_id;
	PoolID pool_id;
};


class DescriptorPool
{
	public:
	DescriptorPool (
	    VkDevice device, DescriptorLayout const& layout, VkDescriptorSetLayout vk_layout, LayoutID layout_id, uint32_t max_sets);
	~DescriptorPool ();
	DescriptorPool (DescriptorPool const& other) = delete;
	DescriptorPool& operator= (DescriptorPool const& other) = delete;
	DescriptorPool (DescriptorPool&& other);
	DescriptorPool& operator= (DescriptorPool&& other);

	DescriptorSet Allocate ();
	void Free (DescriptorSet const& set);

	private:
	struct Pool
	{
		VkDescriptorPool pool;
		uint16_t allocated = 0;
		uint16_t max = 10;
		PoolID id;
	};

	uint32_t AddNewPool ();
	std::optional<DescriptorSet> TryAllocate (Pool& pool);

	std::mutex lock;
	VkDevice device;
	VkDescriptorSetLayout vk_layout;
	LayoutID layout_id;
	uint32_t max_sets;
	std::vector<VkDescriptorPoolSize> pool_members;
	std::vector<Pool> pools;
};

class DescriptorManager
{

	public:
	DescriptorManager (VulkanDevice& device);
	~DescriptorManager ();

	LayoutID CreateDescriptorSetLayout (DescriptorLayout layout);
	void DestroyDescriptorSetLayout (LayoutID id);

	VkDescriptorSetLayout GetLayout (LayoutID id);

	DescriptorSet CreateDescriptorSet (LayoutID layout);
	void DestroyDescriptorSet (DescriptorSet const& set);

	private:
	std::mutex lock;
	VulkanDevice& device;
	LayoutID cur_id = 0;
	std::unordered_set<DescriptorLayout> layout_descriptions;
	std::unordered_map<LayoutID, VkDescriptorSetLayout> layouts;
	std::unordered_map<LayoutID, DescriptorPool> pools;
};