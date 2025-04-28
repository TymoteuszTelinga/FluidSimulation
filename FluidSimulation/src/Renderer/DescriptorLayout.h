#pragma once

#include "Device.h"
#include <Vulkan/vulkan.h>
#include <vector>

class Buffer;
class Image;

struct LayoutSpecification
{
	Device device;
	VkShaderStageFlags stage;
	std::vector<VkDescriptorType> types;
};

class DescriptorLayout
{
public:
	DescriptorLayout() {};
	~DescriptorLayout() {};

	void Init(const LayoutSpecification& spec);
	void Free();

	std::vector<VkDescriptorSet> CreateSets(VkDescriptorPool pool, const uint32_t count, const std::vector<Buffer>& buffers, const std::vector<Image>& images);
	VkDescriptorSetLayout GetHandle() const { return m_Layout; };

private:
	LayoutSpecification m_Spec;
	VkDescriptorSetLayout m_Layout;
};