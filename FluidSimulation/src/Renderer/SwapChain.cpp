#include "SwapChain.h"
#include "Image.h"

VkImage SwapChain::AcquireImage(VkSemaphore& semaphore)
{
	VkResult result = vkAcquireNextImageKHR(m_Spec.device, m_Swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &m_ActiveImage);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		return VK_NULL_HANDLE;
	}
	return m_Images[m_ActiveImage];
}

VkImageView SwapChain::GetImageView(uint32_t index)
{
	if (index < m_ImageViews.size())
	{
		return m_ImageViews[index];
	}
	return VK_NULL_HANDLE;
}

void SwapChain::Init(const SwapChainSpecification& swapChainSpec)
{
	m_Spec = swapChainSpec;

	VkSurfaceCapabilitiesKHR SurfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Spec.device.physicalDevice, m_Spec.surface, &SurfaceCapabilities);

	m_Spec.imagesCount = std::max(SurfaceCapabilities.minImageCount, m_Spec.imagesCount);
	if (SurfaceCapabilities.maxImageCount > 0 && m_Spec.imagesCount > SurfaceCapabilities.maxImageCount)
	{
		m_Spec.imagesCount = SurfaceCapabilities.maxImageCount;
	}

	m_SwapChainExtend = SurfaceCapabilities.currentExtent;

	VkSwapchainCreateInfoKHR SwapChainCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = m_Spec.surface,
		.minImageCount = m_Spec.imagesCount,
		.imageFormat = m_Spec.format.format,
		.imageColorSpace = m_Spec.format.colorSpace,
		.imageExtent = m_SwapChainExtend,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,//VK_IMAGE_USAGE_STORAGE_BIT
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &m_Spec.queueFamily,
		.preTransform = SurfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
		.clipped = VK_TRUE
	};

	vkCreateSwapchainKHR(m_Spec.device, &SwapChainCreateInfo, nullptr, &m_Swapchain);

	m_Images.resize(m_Spec.imagesCount);
	m_ImageViews.resize(m_Spec.imagesCount);
	vkGetSwapchainImagesKHR(m_Spec.device, m_Swapchain, &m_Spec.imagesCount, m_Images.data());

	for (size_t i = 0; i < m_Images.size(); i++)
	{
		m_ImageViews[i] = Image::CreateImageView(m_Spec.device, m_Images[i], VK_IMAGE_VIEW_TYPE_2D, m_Spec.format.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void SwapChain::Resize()
{
	Free();
	Init(m_Spec);
}

void SwapChain::Free()
{
	vkDeviceWaitIdle(m_Spec.device);
	for (auto& imageView : m_ImageViews)
	{
		vkDestroyImageView(m_Spec.device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Spec.device, m_Swapchain, nullptr);
}
