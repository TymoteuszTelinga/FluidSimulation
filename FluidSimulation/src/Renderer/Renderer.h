#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "Core/Base.h"
#include "Core/Window.h"

#include "Image.h"
#include "Buffer.h"

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
	void RenderFrame(float deltaTime);
	void updateUniformBuffer(float deltaTime, uint32_t currentImage);

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
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateComandPool();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	//void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemor);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void ClearSwapChain();

private:
	const uint32_t MAX_IN_FLIGHT_FRAMES = 2;
	//chek for suport of MSAA
	const VkSampleCountFlagBits c_MSAACount = VK_SAMPLE_COUNT_4_BIT;
	//chek if phisical device suport specific format
	const VkSurfaceFormatKHR c_SwapChainFormat =
	{
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	};


	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	PhysicalDevice m_PhysicalDevice;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_Queue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	VkExtent2D m_ImageExtend;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;

	Image m_DepthImage;
	Image m_MSAAImage;
	
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	//buffers
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	std::vector<Buffer> m_UniformBuffers;

	//uniforms info
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	
	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;// = VK_NULL_HANDLE;

	//synkroniacion
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkFence> m_InFlightFences;// = VK_NULL_HANDLE;
};