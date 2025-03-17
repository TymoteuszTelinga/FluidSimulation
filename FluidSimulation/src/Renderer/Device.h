#pragma once

#include <Vulkan/vulkan.h>

struct Device
{
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	operator VkDevice() const { return device; };
};