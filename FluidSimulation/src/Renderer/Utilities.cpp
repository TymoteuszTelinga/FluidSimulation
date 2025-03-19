#include "Utilities.h"

#include "Core/Log.h"
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
