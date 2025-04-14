#include "ComputePipeline.h"
#include "Utilities.h"

void ComputePipeline::Init(const ComputePipelineSpecification& spec)
{
	m_Spec = spec;

	VkShaderModule computeShaderModule = Utilites::CreateShaderModule(m_Spec.device, m_Spec.shaderPath);

	VkPipelineShaderStageCreateInfo compShaderStageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = computeShaderModule,
		.pName = m_Spec.entryPoint.c_str()
	};

	VkPushConstantRange pushConstant =
	{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = m_Spec.constantsSize
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &m_Spec.descriptorLayout,
		.pushConstantRangeCount = m_Spec.constantsSize == 0 ? 0u : 1u,
		.pPushConstantRanges = m_Spec.constantsSize == 0 ? nullptr : &pushConstant,
	};

	vkCreatePipelineLayout(m_Spec.device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

	VkComputePipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = compShaderStageInfo,
		.layout = m_PipelineLayout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	vkCreateComputePipelines(m_Spec.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);

	vkDestroyShaderModule(m_Spec.device, computeShaderModule, nullptr);
}

void ComputePipeline::Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCount, const VkDescriptorSet* descriptorSet, const void* pushData)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, descriptorSet, 0, nullptr);

	if (pushData)
	{
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, m_Spec.constantsSize, pushData);
	}

	vkCmdDispatch(commandBuffer, groupCount, 1, 1);

	VkMemoryBarrier barrier =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
	};

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

}

void ComputePipeline::Free()
{
	vkDestroyPipeline(m_Spec.device, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_Spec.device, m_PipelineLayout, nullptr);
}
