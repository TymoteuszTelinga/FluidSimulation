#pragma once

#include <Vulkan/vulkan.h>
#include "Device.h"

struct ImageSpecification
{
	Device device;
	VkImageType type;
	VkExtent3D size;
	VkFormat format;
	VkImageUsageFlags usage;
	VkImageAspectFlags aspectFlags;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

class Image
{
public:
	Image() {};
	~Image() {};

	VkImage GetImageHandle() const { return m_Image; };
	VkImageView GetImageView() const { return m_ImageView; };

	void Init(const ImageSpecification& imageSpec);
	void Resize(VkExtent2D size);
	VkExtent3D GetExtent() const { return m_Spec.size; }
	void Free();

	static VkImageView CreateImageView(VkDevice Device, VkImage Image, VkImageViewType ViewType, VkFormat Format, VkImageAspectFlags AspectFlags);
	static VkSampler CreateSampler(VkDevice Device);

private:
	void Invalidate();
	VkImageViewType ViewTypeFromImage() const;

private:
	ImageSpecification m_Spec;

	VkImage m_Image = VK_NULL_HANDLE;
	VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
	VkImageView m_ImageView = VK_NULL_HANDLE;
};