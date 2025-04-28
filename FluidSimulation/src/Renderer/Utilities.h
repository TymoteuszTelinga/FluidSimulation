#pragma once

#include <Vulkan/vulkan.h>
#include <filesystem>

namespace Utilites
{
	struct TransferImageDetails
	{
		VkImageLayout oldLayout;
		VkImageLayout newLayout;
		VkAccessFlags srcAccess;
		VkAccessFlags dstAccess;
		VkPipelineStageFlags srcMask;
		VkPipelineStageFlags dstMask;
	};

	const std::string GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity);
	const std::string GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type);

	VkShaderModule CreateShaderModule(VkDevice device, const std::filesystem::path& filepath);

	void CmdTransferImage(VkCommandBuffer commandBuffer, VkImage image, const TransferImageDetails& transferDetails);
	void CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkExtent3D srcExtend, VkImage dstImage, VkExtent3D dstExtend);

};
