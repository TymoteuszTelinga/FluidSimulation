#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "Core/Base.h"
#include "Core/Window.h"

class Renderer
{
	struct PhysicalDevice
	{
		VkPhysicalDevice PhysicalDeviceHandle = VK_NULL_HANDLE;
		std::vector<VkQueueFamilyProperties> QueuesFalimies;

		uint32_t GetFamilyQueue(VkQueueFlags queueType);
		uint32_t GetPresentQueue(VkSurfaceKHR surface);
	};

public:
	Renderer() {};
	~Renderer();

	void Init(const std::string& applicationName, const Scope<Window>& window);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	//temporary solution
	void RenderFrame();

	//Resize function
	void RecreateSwapChain();

private:

	void CreateInstance(const std::string& applicationName);
	void CreateDebugCallback();
	void CreateSurface(const Scope<Window>& window);
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateComandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);

	void ClearSwapChain();


private:
	const int32_t MAX_IN_FLIGHT_FRAMES = 2;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	PhysicalDevice m_PhysicalDevice;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_Queue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	VkExtent2D m_ImageExtend;
	VkSurfaceFormatKHR m_SwapChainFormat;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;
	
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	//uniforms info
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	
	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;// = VK_NULL_HANDLE;

	//synkroniacion
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkFence> m_InFlightFences;// = VK_NULL_HANDLE;
};