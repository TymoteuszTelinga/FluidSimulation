#include "Image.h"

void Image::Init(const ImageSpecification& imageSpec)
{
	m_Spec = imageSpec;
	Invalidate();
}

void Image::Resize(VkExtent2D size)
{
	//if size == 0 || size == old
	if ((size.width == 0 || size.height == 0) || (size.width == m_Spec.size.width && size.height == m_Spec.size.height))
	{
		return;
	}

	m_Spec.size = { size.width, size.height, 1 };

	Invalidate();
}

//free image resources
void Image::Free()
{
	vkDestroyImageView(m_Spec.device, m_ImageView, nullptr);
	vkDestroyImage(m_Spec.device, m_Image, nullptr);
	vkFreeMemory(m_Spec.device, m_ImageMemory, nullptr);
}

//helper funcion for createing image view
VkImageView Image::CreateImageView(VkDevice Device, VkImage Image, VkImageViewType ViewType, VkFormat Format, VkImageAspectFlags AspectFlags)
{
	VkImageViewCreateInfo ViewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = Image,
		.viewType = ViewType,
		.format = Format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = {
			.aspectMask = AspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	VkImageView ImageView;
	vkCreateImageView(Device, &ViewInfo, NULL, &ImageView);
	return ImageView;
}

VkSampler Image::CreateSampler(VkDevice Device)
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.mipLodBias = 0.f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1.f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = 0.f,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	VkSampler sampler;
	vkCreateSampler(Device, &samplerInfo, nullptr, &sampler);
	return sampler;
}

//create new image on GPU
void Image::Invalidate()
{
	if (m_Image != VK_NULL_HANDLE)
	{
		Free();
	}

	VkImageCreateInfo imageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = m_Spec.type,
		.format = m_Spec.format,
		.extent = {m_Spec.size.width, m_Spec.size.height, m_Spec.size.depth},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = m_Spec.samples,
		.tiling = m_Spec.tiling,
		.usage = m_Spec.usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	vkCreateImage(m_Spec.device, &imageInfo, nullptr, &m_Image);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Spec.device, m_Image, &memRequirements);
	uint32_t memType = m_Spec.device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = memType
	};
	vkAllocateMemory(m_Spec.device, &allocInfo, nullptr, &m_ImageMemory);
	vkBindImageMemory(m_Spec.device, m_Image, m_ImageMemory, 0);

	VkImageViewType viewType = ViewTypeFromImage();
	m_ImageView = CreateImageView(m_Spec.device, m_Image, viewType, m_Spec.format, m_Spec.aspectFlags);
}

VkImageViewType Image::ViewTypeFromImage() const
{
	switch (m_Spec.type)
	{
	case VK_IMAGE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
	case VK_IMAGE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
	case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
	}

	return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}
