#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "Core/Base.h"
#include "Core/Window.h"

#include "Image.h"
#include "Buffer.h"
#include "ComputePipeline.h"
#include "DescriptorLayout.h"

class Camera;

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

	void Begin(VkCommandBuffer commandBuffer);
	void End(VkCommandBuffer commandBuffer);

	void SimulationStep(float deltaTime);
	void RenderFrame(float deltaTime);

	void UpdateCamera(const Camera& camera);
	//void updateUniformBuffer(float deltaTime, uint32_t currentImage);

	//Fluid simlualtion steps
	//1. Apply external forcess (gravity, mouse) + predict positions
	//1.5 spartila grid for optimalization
	//2. Calculate density + Calculate near density
	//3. Calculate preshure forses
	//4. viscosity corections
	//5. update positions + colisions resolve
	void RunSortStep(ComputePipeline& sortStep, uint32_t size, uint32_t algorithm);

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
	void CreateComputePipeline();

	void CreateFramebuffers();
	void CreateComandPool();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateInstanceBuffer();
	void CreateUniformBuffers();

	void CreateDescriptorPool();

	void CreateDescriptorSets();
	void CreateComputeDescriptorSets();
	void CreateSortDescriptorSets();

	void CreateCommandBuffer();
	void CreateSyncObjects();

	//void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemor);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void ClearSwapChain();

private:
	const uint32_t MAX_IN_FLIGHT_FRAMES = 1;
	//chek for suport of MSAA
	const VkSampleCountFlagBits c_MSAACount = VK_SAMPLE_COUNT_2_BIT;
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
	VkQueue m_ComputeQueue = VK_NULL_HANDLE;
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
	Buffer m_InstanceBuffer;
	Buffer m_SpatialBuffer;
	Buffer m_LookUpBuffer;
	std::vector<Buffer> m_UniformBuffers;
	Buffer m_SimulationSpecBuffer;

	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

	//Graphics
	//VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	DescriptorLayout m_GraphicsSetLayout;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

	//Compute
	DescriptorLayout m_SimulationSetLayout;
	std::vector<VkDescriptorSet> m_ComputeDescriptorSets;

	ComputePipeline m_GravityStep;
	ComputePipeline m_DensityStep;
	ComputePipeline m_PressureStep;
	ComputePipeline m_ViscosityStep;
	ComputePipeline m_UpdateStep;

	//sorting
	//VkDescriptorSetLayout m_SortDescriptorSetLayout = VK_NULL_HANDLE;
	DescriptorLayout m_SortSetLayout;
	std::vector<VkDescriptorSet> m_SortDescriptorSets;

	ComputePipeline m_SortStep;
	ComputePipeline m_OffsetStep;


	
	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;// = VK_NULL_HANDLE;

	//synkroniacion
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkFence> m_InFlightFences;// = VK_NULL_HANDLE;
};