#include "Utilities.h"

#include "Core/Log.h"
#include "Image.h"
#include <fstream>

static std::vector<char> readFile(const std::filesystem::path& filepath)
{
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file)
	{
		LOG(ERROR, "Open file Error (%s)", filepath.c_str());
	}

	file.seekg(0, std::ios::end);
	uint32_t fileSize = (uint32_t)file.tellg();

	std::vector<char> buffer(fileSize);

	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

const std::string Utilites::GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity)
{
	switch (Severity) 
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "Verbose";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "Info";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "Warning";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "Error";

	default:
		return "UnKnown";
	}

	return "NO SUCH SEVERITY!";
}

const std::string Utilites::GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type)
{
	switch (Type) 
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		return "General";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		return "Validation";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		return "Performance";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		return "Device address binding";

	default:
		return "UnKnown";
	}

	return "NO SUCH TYPE!";
}

VkShaderModule Utilites::CreateShaderModule(VkDevice device, const std::filesystem::path& filepath)
{
	std::vector<char> code = readFile(filepath);

	VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};

	VkShaderModule shaderModule;
	vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	return shaderModule;
}

void Utilites::CmdTransferImage(VkCommandBuffer commandBuffer, VkImage image, const TransferImageDetails& transferDetails)
{
	VkImageMemoryBarrier imageBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = transferDetails.srcAccess,
		.dstAccessMask = transferDetails.dstAccess,
		.oldLayout = transferDetails.oldLayout,
		.newLayout = transferDetails.newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	};

	vkCmdPipelineBarrier(commandBuffer, transferDetails.srcMask, transferDetails.dstMask, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

void Utilites::CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkExtent3D srcExtend, VkImage dstImage, VkExtent3D dstExtend)
{

	VkImageBlit blitRegion = {};
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcOffsets[0] = { 0, 0, 0 };
	blitRegion.srcOffsets[1] = { (int)srcExtend.width, (int)srcExtend.height, (int)srcExtend.depth };
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.mipLevel = 0;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstOffsets[0] = { 0, 0, 0 };
	blitRegion.dstOffsets[1] = { (int)dstExtend.width, (int)dstExtend.height, (int)dstExtend.depth };

	vkCmdBlitImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
}
