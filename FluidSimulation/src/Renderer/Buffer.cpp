#include "Buffer.h"
#include <cstring>

static void CreateBuffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemor)
{
	//init buffer handle
	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

	//get and find best memory type
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	uint32_t memType = device.FindMemoryType(memRequirements.memoryTypeBits, properties);

	//AlocateMemeory
	VkMemoryAllocateInfo allocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = memType
	};
	vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemor);
	vkBindBufferMemory(device, buffer, bufferMemor, 0);
}

void Buffer::Init(const BufferSpecification& bufferSpec, const void* data)
{
	m_Spec = bufferSpec;
	//create main buffer
	CreateBuffer(m_Spec.device, m_Spec.size, m_Spec.usage, m_Spec.properties, m_Buffer, m_BufferMemory);
	if (data || m_Spec.AllowWrite)
	{
		vkMapMemory(m_Spec.device, m_BufferMemory, 0, m_Spec.size, 0, &m_MemoryMapped);
	}
	if (data)
	{
		memcpy(m_MemoryMapped, data, m_Spec.size);
	}
	if (data && !m_Spec.AllowWrite)
	{
		vkUnmapMemory(m_Spec.device, m_BufferMemory);
		m_MemoryMapped = nullptr;
	}


}

void Buffer::Write(const void* data, uint32_t size)
{
	memcpy(m_MemoryMapped, data, size);
}

void Buffer::Free()
{
	vkDestroyBuffer(m_Spec.device, m_Buffer, nullptr);
	vkFreeMemory(m_Spec.device, m_BufferMemory, nullptr);
	m_MemoryMapped = nullptr;
}
