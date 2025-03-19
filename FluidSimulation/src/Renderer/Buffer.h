#pragma once

#include "Device.h"
#include <Vulkan/vulkan.h>

struct BufferSpecification
{
	Device device;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags properties;
	bool AllowWrite = false;
};

class Buffer
{
public:
	Buffer() {};
	~Buffer() {};

	void Init(const BufferSpecification& bufferSpec, const void* data);
	void Write(const void* data, uint32_t size);
	void Free();

	VkBuffer GetHandle() const { return m_Buffer; };

private:
	BufferSpecification m_Spec;

	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
	void* m_MemoryMapped = nullptr;
};
