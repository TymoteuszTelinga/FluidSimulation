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
	void CreateColorResources();
	void CreateDepthResources();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	void CreateImage(VkFormat format, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemor);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


	void ClearSwapChain();


private:
	const uint32_t MAX_IN_FLIGHT_FRAMES = 2;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	PhysicalDevice m_PhysicalDevice;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_Queue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	VkExtent2D m_ImageExtend;
	VkSurfaceFormatKHR m_SwapChainFormat;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;

	VkImage m_DepthImage = VK_NULL_HANDLE;
	VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
	VkImageView m_DepthImageView = VK_NULL_HANDLE;

	const VkSampleCountFlagBits c_MSAACount = VK_SAMPLE_COUNT_4_BIT;
	VkImage m_MSAAImage = VK_NULL_HANDLE;
	VkDeviceMemory m_MSAAImageMemory = VK_NULL_HANDLE;
	VkImageView m_MSAAImageView = VK_NULL_HANDLE;

	
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	//buffers
	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;

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