#pragma once

#include <Vulkan/vulkan.h>
#include <filesystem>

namespace Utilites
{
	const std::string GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity);
	const std::string GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type);

	VkShaderModule CreateShaderModule(VkDevice device, const std::filesystem::path& filepath);
};
