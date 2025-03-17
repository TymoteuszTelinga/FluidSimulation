#pragma once

#include <Vulkan/vulkan.h>
#include "Device.h"

struct ImageSpecification
{
	Device device;
	VkExtent2D size;
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
	void Free();

	static VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);

private:
	void Invalidate();

private:
	ImageSpecification m_Spec;

	VkImage m_Image = VK_NULL_HANDLE;
	VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
	VkImageView m_ImageView = VK_NULL_HANDLE;
};