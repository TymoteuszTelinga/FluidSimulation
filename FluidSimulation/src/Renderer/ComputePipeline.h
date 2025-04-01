#pragma once

#include "Device.h"
#include <filesystem>
#include <Vulkan/vulkan.h>

struct ComputePipelineSpecification
{
	Device device;
	std::filesystem::path shaderPath;
	std::string entryPoint;
	VkDescriptorSetLayout descriptorLayout;
	uint32_t constantsSize;
};

class ComputePipeline
{
public:
	ComputePipeline() {};
	~ComputePipeline() {};

	VkPipeline GetHandle() const { return m_Pipeline; };
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; };

	void Init(const ComputePipelineSpecification& spec);
	void Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCount, const VkDescriptorSet* descriptorSet, const void* pushData = nullptr);
	void Free();

private:
	ComputePipelineSpecification m_Spec;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
};