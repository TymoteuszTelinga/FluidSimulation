#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "Core/Base.h"
#include "Core/Window.h"
#include <glm/glm.hpp>

#include "Image.h"
#include "Buffer.h"
#include "ComputePipeline.h"
#include "DescriptorLayout.h"
#include "SwapChain.h"
#include "Device.h"

class Camera;

struct Object
{
	alignas(16) glm::mat4 transform;
	alignas(16) glm::vec3 Color;
};

class Renderer
{

public:
	Renderer() {};
	~Renderer();

	void Init(const std::string& applicationName, const Scope<Window>& window);
	void Wait() const;

	//temporary solution

	void Begin(VkCommandBuffer commandBuffer);
	void End(VkCommandBuffer commandBuffer, VkPipelineStageFlags *waitStages = nullptr, const std::vector<VkSemaphore>& waitSemaphores = {}, const std::vector<VkSemaphore>& signalSemaphores = {});

	void SimulationStep(float deltaTime);
	void RestartSimulation();
	void RenderFrame(float deltaTime);
	void Present();

	void UpdateCamera(const Camera& camera);
	void UpdateRenderSetings(float stepSize, bool bDynamic, float sampling);
	void UpdateSceneObjects(const std::vector<Object>& objects);
	//void updateUniformBuffer(float deltaTime, uint32_t currentImage);

	//Fluid simlualtion steps
	//1. Apply external forcess (gravity, mouse) + predict positions
	//1.5 spartila grid for optimalization
	//2. Calculate density + Calculate near density
	//3. Calculate preshure forses
	//4. viscosity corections
	//5. update positions + colisions resolve
	void RunSortStep(ComputePipeline& sortStep, uint32_t size, uint32_t algorithm);

	//geter function for ImGUI
	VkInstance GetInstance() const { return m_Instance; };
	Device GetDevice() const { return m_Device; };
	uint32_t GetQueueFamily() const { return m_QueueFamily; };
	VkQueue GetQueue() const { return m_Queue; };
	VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; };
	VkRenderPass GetRenderPass() const { return m_GUIPass; };

	friend void RenderImGui(Scope<Renderer>& renderer);
private:

	void CreateInstance(const std::string& applicationName);
	void CreateDebugCallback();
	void CreateSurface(const Scope<Window>& window);
	void CreateDescriptorSetLayout();

	void CreateComputePipeline();

	void CreateComandPool();

	void CreateInstanceBuffer();
	void CreateUniformBuffers();

	void CreateDescriptorPool();
	void CreateGUIPass();

	void CreateComputeDescriptorSets();
	void CreateSortDescriptorSets();

	void InitRayTracing();

	void CreateCommandBuffer();
	void CreateSyncObjects();

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
	const uint32_t MAX_IN_FLIGHT_FRAMES = 1;
	//chek for suport of MSAA
	const VkSampleCountFlagBits c_MSAACount = VK_SAMPLE_COUNT_1_BIT;
	//chek if phisical device suport specific format
	const VkSurfaceFormatKHR c_SwapChainFormat =
	{
		.format = VK_FORMAT_R8G8B8A8_UNORM,//VK_FORMAT_R8G8B8A8_SRGB,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	};

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	Device m_Device;

	uint32_t m_QueueFamily = -1;
	VkQueue m_ComputeQueue = VK_NULL_HANDLE;
	VkQueue m_Queue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	SwapChain m_SwapChain;
	VkExtent2D m_ImageExtend;
	VkRenderPass m_GUIPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_Framebuffers;


	Buffer m_InstanceBuffer;
	Buffer m_SpatialBuffer;
	Buffer m_LookUpBuffer;
	std::vector<Buffer> m_UniformBuffers;
	Buffer m_SimulationSpecBuffer;

	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

	//Compute
	Image m_DensityVolume;
	DescriptorLayout m_SimulationSetLayout;
	std::vector<VkDescriptorSet> m_ComputeDescriptorSets;

	ComputePipeline m_GravityStep;
	ComputePipeline m_DensityStep;
	ComputePipeline m_PressureStep;
	ComputePipeline m_ViscosityStep;
	ComputePipeline m_UpdateStep;
	ComputePipeline m_VolumeStep;

	//sorting
	DescriptorLayout m_SortSetLayout;
	std::vector<VkDescriptorSet> m_SortDescriptorSets;

	ComputePipeline m_SortStep;
	ComputePipeline m_OffsetStep;

	//Graphics
	Image m_ScreenImage;

	DescriptorLayout m_RayTracingLayout;
	VkDescriptorSet m_RayTracingSet;

	Buffer m_ObjectTracing;

	ComputePipeline m_RayTracing;

	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;// = VK_NULL_HANDLE;

	//synkroniacion
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;// = VK_NULL_HANDLE;
	std::vector<VkFence> m_InFlightFences;// = VK_NULL_HANDLE;
};