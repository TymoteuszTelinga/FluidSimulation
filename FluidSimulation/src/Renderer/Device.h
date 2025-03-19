#pragma once

#include <Vulkan/vulkan.h>

struct Device
{
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	operator VkDevice() const { return device; };
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
};