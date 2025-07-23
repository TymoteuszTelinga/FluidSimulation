#pragma once

#include <Vulkan/vulkan.h>

struct Device
{
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	operator VkDevice() const { return device; };
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	static Device CreateDivice(const VkInstance& instance);
	static uint32_t GetFamilyQueue(VkPhysicalDevice device, VkQueueFlags queueType);
	static uint32_t GetPresentQueue(VkPhysicalDevice device, VkSurfaceKHR surface);
};

