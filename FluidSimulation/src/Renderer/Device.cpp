
#include "Device.h"
#include "Core/Log.h"
#include <vector>

uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	//LOG(CRITICAL, "Feiled to find memory type");
}

uint32_t Device::GetFamilyQueue(VkPhysicalDevice device, VkQueueFlags queueType)
{
	uint32_t QFamyliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &QFamyliesCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueuesFalimies(QFamyliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &QFamyliesCount, QueuesFalimies.data());

	for (size_t i = 0; i < QueuesFalimies.size(); i++)
	{
		if (QueuesFalimies[i].queueFlags & queueType)
		{
			return i;
		}
	}

	return 0;
}

uint32_t Device::GetPresentQueue(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t QFamyliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &QFamyliesCount, nullptr);

	for (size_t i = 0; i < QFamyliesCount; i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport)
		{
			return i;
		}
	}
	return 0;
}

Device Device::CreateDivice(const VkInstance& instance)
{
	Device SimulationDevice;
	//Pick Physical device
	{

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	LOG(INFO, "Devices deteted: %d", deviceCount);


	for (auto& deviceHandle : devices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(deviceHandle, &deviceProperties);
		LOG(INFO, "Device name: %s", deviceProperties.deviceName);
		LOG(INFO, "\tAPI version: %d.%d.%d.%d", VK_API_VERSION_VARIANT(deviceProperties.apiVersion), VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			SimulationDevice.physicalDevice = deviceHandle;

			uint32_t QFamyliesCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &QFamyliesCount, nullptr);

			std::vector<VkQueueFamilyProperties> QueuesFalimies(QFamyliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &QFamyliesCount, QueuesFalimies.data());


			for (auto& FamilyProperiy : QueuesFalimies)
			{
				VkQueueFlags Flags = FamilyProperiy.queueFlags;
				LOG(INFO, "\tNum queues: %d\tGFX %s, Compute %s, Transfer %s, Sparse binding %s",
					FamilyProperiy.queueCount, 
					(Flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");
			}
		}

	}

	}

	if (SimulationDevice.physicalDevice == VK_NULL_HANDLE)
	{
		LOG(CRITICAL, "Invalid Physical devices");
		return SimulationDevice;
	}

	//Create Logical Device
	{
		float queuePriority[] = { 1.0f };

		VkDeviceQueueCreateInfo queueCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = GetFamilyQueue(SimulationDevice.physicalDevice, VK_QUEUE_GRAPHICS_BIT),
			.queueCount = 1,
			.pQueuePriorities = &queuePriority[0]
		};

		std::vector<const char*> DevExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
		};

		VkPhysicalDeviceFeatures deviceFeatures = { 0 };

		VkDeviceCreateInfo deviceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = (uint32_t)DevExtensions.size(),
			.ppEnabledExtensionNames = DevExtensions.data(),
			.pEnabledFeatures = &deviceFeatures
		};

		vkCreateDevice(SimulationDevice.physicalDevice, &deviceCreateInfo, nullptr, &SimulationDevice.device);
	}


	return SimulationDevice;
}
