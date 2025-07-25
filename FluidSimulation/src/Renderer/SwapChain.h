#pragma once

#include "Device.h"
#include <Vulkan/vulkan.h>
#include <vector>

struct SwapChainSpecification
{
	Device device;
	VkSurfaceKHR surface;
	VkSurfaceFormatKHR format;
	uint32_t imagesCount;
	uint32_t queueFamily;
};

class SwapChain
{
public:
	SwapChain() {};
	~SwapChain() {};

	VkExtent2D GetExtend() const { return m_SwapChainExtend; };
	VkImage AcquireImage(VkSemaphore& semaphore);
	VkImageView GetImageView(uint32_t index);
	uint32_t GetActiveImageIndex() const { return m_ActiveImage; };
	VkSwapchainKHR GetSwapChain() const { return m_Swapchain; }
	uint32_t GetImageCount() const { return m_Spec.imagesCount; };

	void Init(const SwapChainSpecification& swapChainSpec);
	void Resize();
	void Free();

private:

	SwapChainSpecification m_Spec;
	VkExtent2D m_SwapChainExtend;
	uint32_t m_ActiveImage = 0;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;

	/*std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;*/
};