#include "DescriptorLayout.h"
#include "Buffer.h"

void DescriptorLayout::Init(const LayoutSpecification& spec)
{
	m_Spec = spec;

	std::vector<VkDescriptorSetLayoutBinding> bindings(m_Spec.types.size());

	for (size_t i = 0; i < m_Spec.types.size(); i++)
	{
		bindings[i].binding = i;
		bindings[i].descriptorType = m_Spec.types[i];
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = m_Spec.stage;
		bindings[i].pImmutableSamplers = nullptr;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings = bindings.data()
	};

	vkCreateDescriptorSetLayout(m_Spec.device, &layoutInfo, nullptr, &m_Layout);
}

void DescriptorLayout::Free()
{
	vkDestroyDescriptorSetLayout(m_Spec.device, m_Layout, nullptr);
}

std::vector<VkDescriptorSet> DescriptorLayout::CreateSets(VkDescriptorPool pool, const uint32_t count, const std::vector<Buffer>& buffers)
{
	std::vector<VkDescriptorSetLayout> layouts(count, m_Layout);

	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = pool,
		.descriptorSetCount = count,
		.pSetLayouts = layouts.data()
	};

	std::vector<VkDescriptorSet> sets(count);

	vkAllocateDescriptorSets(m_Spec.device, &allocInfo, sets.data());


	std::vector<VkDescriptorBufferInfo> bufferInfo(buffers.size());
	std::vector<VkWriteDescriptorSet> writes(buffers.size());
	for (size_t s = 0; s < count; s++)
	{
		for (size_t i = 0; i < buffers.size(); i++)
		{
			//info
			bufferInfo[i].buffer = buffers[i].GetHandle();
			bufferInfo[i].offset = 0;
			bufferInfo[i].range  = buffers[i].GetSize();

			//write
			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].pNext = nullptr;
			writes[i].dstSet = sets[s];
			writes[i].dstBinding = i;
			writes[i].dstArrayElement = 0;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = m_Spec.types[i];
			writes[i].pImageInfo = nullptr;
			writes[i].pBufferInfo = &bufferInfo[i];
			writes[i].pTexelBufferView = nullptr;
		}

		vkUpdateDescriptorSets(m_Spec.device, writes.size(), writes.data(), 0, nullptr);
	}

	return sets;
}
