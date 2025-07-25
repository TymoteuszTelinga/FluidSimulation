#include "Renderer.h"

#include <iostream>
#include <fstream>

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Utilities.h"

static struct Instance
{
	alignas(16) glm::vec3 position;
	alignas(4)	float density;
	alignas(16) glm::vec3 velocity;
	alignas(16) glm::vec3 predictedPosition;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription description =
		{
			.binding = 1,
			.stride = sizeof(Instance),
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE

		};
		return description;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAtributeDescription()
	{
		std::vector<VkVertexInputAttributeDescription> descriptions;

		//position attrib
		{
			VkVertexInputAttributeDescription InstanceAttribute =
			{
				.location = 3,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Instance, position)
			};

			descriptions.push_back(InstanceAttribute);
		}

		{
			VkVertexInputAttributeDescription InstanceAttribute =
			{
				.location = 4,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Instance, velocity)
			};

			descriptions.push_back(InstanceAttribute);
		}


		return descriptions;
	}
};

static std::vector<Instance> instances;

static struct UniformBufferObject
{
	alignas(16) glm::mat4 model = glm::mat4(1.f);
	alignas(16) glm::mat4 view  = glm::mat4(1.f);
	alignas(16) glm::mat4 proj  = glm::mat4(1.f);
	alignas(16) glm::vec3 cameraPos = glm::vec3(0.f);
	float StepSize = 0.02f;
} s_RenderingUBO;

static struct SimulationUBO
{
	float simulationWidth = 26.f / 2.f;//in use
	float simulationHeight = 26.f / 2.f;//in use
	float simulationDepth = 10.f/ 2.f;//in use
	float collisionDamping = 0.8f;//in use
	float kernelRadius = 0.3;//in use
	float targetDensity = 105.75f;//in use
	float pressureMultiplier = 300.5f;//in use
	uint32_t particlesCount = 256 * (128);//in use (256 * n) (128 + 64 + 16 + 8)
} s_SimUBO;

Renderer::~Renderer()
{
	Wait();

	for (auto& semaphore : m_ImageAvailableSemaphores)
	{
		vkDestroySemaphore(m_Device, semaphore, nullptr);
	}
	for (auto& semaphore : m_RenderFinishedSemaphores)
	{
		vkDestroySemaphore(m_Device, semaphore, nullptr);
	}
	for (auto& fence : m_InFlightFences)
	{
		vkDestroyFence(m_Device, fence, nullptr);
	}

	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

	m_SimulationSetLayout.Free();
	m_SortSetLayout.Free();
	m_RayTracingLayout.Free();

	m_InstanceBuffer.Free();
	m_SpatialBuffer.Free();
	m_LookUpBuffer.Free();

	for (auto& buffer : m_UniformBuffers)
	{
		buffer.Free();
	}
	m_SimulationSpecBuffer.Free();

	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	m_GravityStep.Free();
	m_DensityStep.Free();
	m_PressureStep.Free();
	m_ViscosityStep.Free();
	m_UpdateStep.Free();
	m_VolumeStep.Free();

	m_SortStep.Free();
	m_OffsetStep.Free();

	m_RayTracing.Free();

	//ClearSwapChain();
	m_SwapChain.Free();
	vkDestroyRenderPass(m_Device, m_GUIPass, nullptr);
	for (auto& framebuffer : m_Framebuffers)
	{
		vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
	}

	m_DensityVolume.Free();
	m_ScreenImage.Free();

	vkDestroyDevice(m_Device, nullptr);

	PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
	vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(m_Instance, "vkDestroySurfaceKHR");
	vkDestroySurface(m_Instance, m_Surface, nullptr);

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, nullptr);
	
	vkDestroyInstance(m_Instance, nullptr);
}

void Renderer::Init(const std::string& applicationName, const Scope<Window>& window)
{
	instances.clear();

	float minZ = 0.f;
	float maxZ = 0.f;
	for (size_t i = 0; i < s_SimUBO.particlesCount; i++)
	{
		float x = (float)rand() / RAND_MAX;
		float y = (float)rand() / RAND_MAX;
		float z = (float)rand() / RAND_MAX;
		x = x * 2.f - 1.f;
		y = y * 2.f - 1.f;
		z = z * 2.f - 1.f;
		x *= s_SimUBO.simulationDepth;//s_SimUBO.simulationWidth;
		y *= s_SimUBO.simulationDepth;//s_SimUBO.simulationHeight;
		z *= s_SimUBO.simulationDepth;

		minZ = std::min(z, minZ);
		maxZ = std::max(z, maxZ);

		x += 0.f;
		y += s_SimUBO.simulationHeight - s_SimUBO.simulationDepth;
		z += 0.f;

		Instance particle;
		particle.position = glm::vec3(x, y, z);
		particle.predictedPosition = particle.position;
		particle.density = 0.0f;
		particle.velocity = glm::vec3(0.f,0.f,0.f);
		instances.push_back(particle);
	}

	CreateInstance(applicationName);
	CreateDebugCallback();
	CreateSurface(window);

	m_Device = Device::CreateDivice(m_Instance);
	m_QueueFamily = Device::GetFamilyQueue(m_Device.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_ComputeQueue);
	vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_Queue); //graphics
	uint32_t PresentQueue = Device::GetPresentQueue(m_Device.physicalDevice, m_Surface);
	vkGetDeviceQueue(m_Device, PresentQueue, 0, &m_PresentQueue);

	//CreateSwapChain();
	SwapChainSpecification SwapchainSpec =
	{
		.device = m_Device,
		.surface = m_Surface,
		.format = c_SwapChainFormat,
		.imagesCount = 3,
		.queueFamily = m_QueueFamily
	};
	m_SwapChain.Init(SwapchainSpec);
	m_ImageExtend = m_SwapChain.GetExtend();
	CreateGUIPass();

	CreateDescriptorSetLayout();
	CreateComputePipeline();

	CreateComandPool();

	ImageSpecification imageSpec =
	{
		.device = m_Device,
		.type = VK_IMAGE_TYPE_3D,
		.size = {88*2, 88*2, 32*2},
		.format = VK_FORMAT_R32_SFLOAT,
		.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL
	};
	m_DensityVolume.Init(imageSpec);

	CreateInstanceBuffer();
	CreateUniformBuffers();

	CreateDescriptorPool();

	CreateComputeDescriptorSets();
	CreateSortDescriptorSets();

	InitRayTracing();

	CreateCommandBuffer();
	CreateSyncObjects();

	LOG(DEBUG, "Active Particles: %u", s_SimUBO.particlesCount);
}

void Renderer::Wait() const
{
	vkDeviceWaitIdle(m_Device);
}

void Renderer::CreateInstance(const std::string& applicationName)
{
	std::vector<const char*> Layers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> Extensions =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		"VK_KHR_win32_surface",
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	VkApplicationInfo AppInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = applicationName.c_str(),
		.applicationVersion = VK_MAKE_API_VERSION(0,1,0,0),
		.pEngineName = "SPH Rendering",
		.engineVersion = VK_MAKE_API_VERSION(0,1,0,0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo InstanceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &AppInfo,
		.enabledLayerCount = (uint32_t)Layers.size(),
		.ppEnabledLayerNames = Layers.data(),
		.enabledExtensionCount = (uint32_t)Extensions.size(),
		.ppEnabledExtensionNames = Extensions.data()
	};

	VkResult res = vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance);
	if (res != VK_SUCCESS)
	{
		LOG(CRITICAL, "Instance creation Error")
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	printf("Debug Callback:\n");
	printf("   Severity: %s\n", Utilites::GetDebugSeverityStr(messageSeverity).c_str());
	printf("   Type: %s\n", Utilites::GetDebugType(messageTypes).c_str());
	printf("   Message: %s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

void Renderer::CreateDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = nullptr
	};

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

	VkResult res = vkCreateDebugUtilsMessenger(m_Instance, &MessengerCreateInfo, nullptr, &m_DebugMessenger);
	if (res != VK_SUCCESS)
	{
		LOG(WARNING, "DebugCallback creation Error");
	}
}

void Renderer::CreateSurface(const Scope<Window>& window)
{
	if (glfwCreateWindowSurface(m_Instance, window->GetWindow(), nullptr, &m_Surface))
	{
		LOG(CRITICAL, "Surface creattion error");
	}
}

void Renderer::CreateDescriptorSetLayout()
{
	//Simulation
	LayoutSpecification simulationLayoutSpec =
	{
		.device = m_Device,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}
		//uniform, particles, spacial, lookup, volumeImage
	};
	m_SimulationSetLayout.Init(simulationLayoutSpec);

	//sorting
	LayoutSpecification sortLayoutSpec =
	{
		.device = m_Device,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.types = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}
		//spatial, lookup
	};
	m_SortSetLayout.Init(sortLayoutSpec);
}

void Renderer::CreateComputePipeline()
{
	ComputePipelineSpecification gravitySpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "ExternalForce",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_GravityStep.Init(gravitySpec);
	
	ComputePipelineSpecification densitySpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "DensityStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_DensityStep.Init(densitySpec);

	ComputePipelineSpecification pressureSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "PressureStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_PressureStep.Init(pressureSpec);

	ComputePipelineSpecification viscositySpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "ViscosityStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_ViscosityStep.Init(viscositySpec);

	ComputePipelineSpecification updateSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "UpdateStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_UpdateStep.Init(updateSpec);

	ComputePipelineSpecification sortSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Sorting\\Sort.spv",
		.entryPoint = "BitonicSort",
		.descriptorLayout = m_SortSetLayout.GetHandle(),
		.constantsSize = sizeof(uint32_t) * 3
	};
	m_SortStep.Init(sortSpec);

	ComputePipelineSpecification offsetSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Sorting\\Sort.spv",
		.entryPoint = "CalculateOffsets",
		.descriptorLayout = m_SortSetLayout.GetHandle(),
		.constantsSize = 0
	};
	m_OffsetStep.Init(offsetSpec);

	ComputePipelineSpecification volumeSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Rendering\\volume.spv",
		.entryPoint = "main",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = 0
	};
	m_VolumeStep.Init(volumeSpec);
}

void Renderer::CreateComandPool()
{
	VkCommandPoolCreateInfo commandPoolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_QueueFamily
	};

	vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_CommandPool);
}

void Renderer::CreateInstanceBuffer()
{
	VkDeviceSize bufferSize = sizeof(Instance) * instances.size();

	BufferSpecification InstanceSpec =
	{
		.device = m_Device,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_InstanceBuffer.Init(InstanceSpec, nullptr);

	BufferSpecification stagingSpec =
	{
		.device = m_Device,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer stagingBuffer;
	stagingBuffer.Init(stagingSpec, instances.data());

	CopyBuffer(stagingBuffer.GetHandle(), m_InstanceBuffer.GetHandle(), bufferSize);

	stagingBuffer.Free();

	BufferSpecification SpatialSpec =
	{
		.device = m_Device,
		.size = sizeof(uint32_t) * instances.size() * 2,//key, Particle index
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_SpatialBuffer.Init(SpatialSpec, nullptr);

	BufferSpecification LookupSpec =
	{
		.device = m_Device,
		.size = sizeof(uint32_t) * instances.size(),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_LookUpBuffer.Init(LookupSpec, nullptr);
}

void Renderer::CreateUniformBuffers()
{
	m_UniformBuffers.resize(MAX_IN_FLIGHT_FRAMES);

	BufferSpecification uniformSpec =
	{
		.device = m_Device,
		.size = sizeof(UniformBufferObject),
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.AllowWrite = true
	};

	for (auto& buffer : m_UniformBuffers) 
	{
		buffer.Init(uniformSpec, &s_RenderingUBO);
	}


	BufferSpecification simulatinUBO =
	{
		.device = m_Device,
		.size = sizeof(SimulationUBO),
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.AllowWrite = true
	};
	m_SimulationSpecBuffer.Init(simulatinUBO, &s_SimUBO);
}

//create pool for uniform buffers and SSBO
void Renderer::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[4] =
	{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = MAX_IN_FLIGHT_FRAMES
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = MAX_IN_FLIGHT_FRAMES
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = MAX_IN_FLIGHT_FRAMES
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 8
		}
	};

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = MAX_IN_FLIGHT_FRAMES * 3 + 8,
		.poolSizeCount = 4,
		.pPoolSizes = poolSizes,
	};

	vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);
}

void Renderer::CreateGUIPass()
{
	VkAttachmentDescription colorAttachment =
	{
		.flags = 0,
		.format = c_SwapChainFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,// VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_GUIPass);

	m_Framebuffers.resize(m_SwapChain.GetImageCount());

	for (size_t i = 0; i < m_SwapChain.GetImageCount(); i++)
	{
		VkImageView attachments[1] = { m_SwapChain.GetImageView(i) };
		VkFramebufferCreateInfo framebufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = m_GUIPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = m_ImageExtend.width,
			.height = m_ImageExtend.height,
			.layers = 1
		};

		vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]);
	}
}

void Renderer::CreateComputeDescriptorSets()
{
	m_ComputeDescriptorSets = m_SimulationSetLayout.CreateSets(m_DescriptorPool, MAX_IN_FLIGHT_FRAMES, { m_SimulationSpecBuffer, m_InstanceBuffer, m_SpatialBuffer, m_LookUpBuffer }, {m_DensityVolume});
}

void Renderer::CreateSortDescriptorSets()
{
	m_SortDescriptorSets = m_SortSetLayout.CreateSets(m_DescriptorPool, MAX_IN_FLIGHT_FRAMES, { m_SpatialBuffer, m_LookUpBuffer }, {});
}

void Renderer::InitRayTracing()
{
	ImageSpecification ScreenSpec =
	{
		.device = m_Device,
		.type = VK_IMAGE_TYPE_2D,
		.size = {m_ImageExtend.width, m_ImageExtend.height, 1},
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL
	};
	m_ScreenImage.Init(ScreenSpec);

	LayoutSpecification RayTracingLayoutSpec =
	{
		.device = m_Device,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.types = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}
		//image, Density, cameraUBO
	};
	m_RayTracingLayout.Init(RayTracingLayoutSpec);

	ComputePipelineSpecification RayTracingSpec =
	{
		.device = m_Device,
		.shaderPath = "src\\Shaders\\Rendering\\basic.spv",
		.entryPoint = "main",
		.descriptorLayout = m_RayTracingLayout.GetHandle(),
		.constantsSize = 0
	};
	m_RayTracing.Init(RayTracingSpec);


	m_RayTracingSet = m_RayTracingLayout.CreateSets(m_DescriptorPool, 1, {m_UniformBuffers[0]}, {m_ScreenImage, m_DensityVolume})[0];
}

void Renderer::CreateCommandBuffer()
{
	m_CommandBuffers.resize(MAX_IN_FLIGHT_FRAMES);

	VkCommandBufferAllocateInfo allocationInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)m_CommandBuffers.size()
	};

	vkAllocateCommandBuffers(m_Device, &allocationInfo, m_CommandBuffers.data());
}

void Renderer::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};

	VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	m_ImageAvailableSemaphores.resize(MAX_IN_FLIGHT_FRAMES);
	m_RenderFinishedSemaphores.resize(MAX_IN_FLIGHT_FRAMES);
	m_InFlightFences.resize(MAX_IN_FLIGHT_FRAMES);

	for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++)
	{
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]);
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]);
		vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]);
	}
}

void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	//create commadBuffer
	VkCommandBufferAllocateInfo allocationInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device, &allocationInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	//Record coppy commad
	vkBeginCommandBuffer(commandBuffer, &beginBufferInfo);

	VkBufferCopy copyRegion =
	{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	//submit and run commandBuffer(copy commad)
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_Queue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}

static uint32_t curentFrame = 0;

void Renderer::Begin(VkCommandBuffer commandBuffer)
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(commandBuffer, &beginBufferInfo);
}

void Renderer::End(VkCommandBuffer commandBuffer, VkPipelineStageFlags* waitStages, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo computeSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = (uint32_t)waitSemaphores.size(),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = (uint32_t)signalSemaphores.size(),
		.pSignalSemaphores = signalSemaphores.data()
	};
	vkQueueSubmit(m_ComputeQueue, 1, &computeSubmitInfo, m_InFlightFences[curentFrame]);
}

void Renderer::SimulationStep(float deltaTime)
{
	deltaTime = std::min(deltaTime, 1.f / 60.f);
	Begin(m_CommandBuffers[curentFrame]);
	m_GravityStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, 1, 1, &m_ComputeDescriptorSets[0], &deltaTime);

	//sorting
	uint32_t steps = s_SimUBO.particlesCount / 256;
	for (size_t i = 0; i < 200; i++)
	{
		if (steps <= std::pow(2, i))
		{
			steps = std::pow(2, i);
			break;
		}
	}

	steps *= 256;
	uint32_t h = 256;
	//BMS
	RunSortStep(m_SortStep, h, 0);//local BMS
	h *= 2;
	for (; h <= steps; h *= 2)
	{
		RunSortStep(m_SortStep, h, 1);//Global flip

		for (uint32_t hh = h / 2; hh > 1; hh /= 2)
		{

			if (hh <= 256)
			{
				RunSortStep(m_SortStep, hh, 3);
				break;
			}
			else
			{
				RunSortStep(m_SortStep, hh, 2);//global dips
			}
		}
	}

	m_OffsetStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 128, 1, 1, &m_SortDescriptorSets[0]);
	m_DensityStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, 1, 1, &m_ComputeDescriptorSets[0], &deltaTime);
	m_PressureStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, 1, 1, &m_ComputeDescriptorSets[0], &deltaTime);
	m_ViscosityStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, 1, 1, &m_ComputeDescriptorSets[0], &deltaTime);
	m_UpdateStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, 1, 1, &m_ComputeDescriptorSets[0], &deltaTime);

	End(m_CommandBuffers[curentFrame]);
}

void Renderer::RestartSimulation()
{
	VkDeviceSize bufferSize = sizeof(Instance) * instances.size();

	BufferSpecification stagingSpec =
	{
		.device = m_Device,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer stagingBuffer;
	stagingBuffer.Init(stagingSpec, instances.data());

	CopyBuffer(stagingBuffer.GetHandle(), m_InstanceBuffer.GetHandle(), bufferSize);

	stagingBuffer.Free();
}

void Renderer::RenderFrame(float deltaTime)
{
	//render image
	Begin(m_CommandBuffers[curentFrame]);

	Utilites::TransferImageDetails volumeBarier =
	{
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.srcAccess = 0,
		.dstAccess = VK_ACCESS_SHADER_WRITE_BIT,
		.srcMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		.dstMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	};
	Utilites::CmdTransferImage(m_CommandBuffers[curentFrame], m_DensityVolume.GetImageHandle(), volumeBarier);
	VkExtent3D extend = m_DensityVolume.GetExtent();
	m_VolumeStep.Dispatch(m_CommandBuffers[curentFrame], extend.width / 8, extend.height / 8, extend.depth / 8, &m_ComputeDescriptorSets[0], nullptr);

	Utilites::TransferImageDetails samplerBarier =
	{
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcAccess = 0,
		.dstAccess = VK_ACCESS_SHADER_WRITE_BIT,
		.srcMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		.dstMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	};
	Utilites::CmdTransferImage(m_CommandBuffers[curentFrame], m_DensityVolume.GetImageHandle(), samplerBarier);

	Utilites::TransferImageDetails barrier =
	{
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.srcAccess = 0,
		.dstAccess = VK_ACCESS_SHADER_WRITE_BIT,
		.srcMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		.dstMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	};
	Utilites::CmdTransferImage(m_CommandBuffers[curentFrame], m_ScreenImage.GetImageHandle(), barrier);

	m_RayTracing.Dispatch(m_CommandBuffers[curentFrame], m_ImageExtend.width / 8, m_ImageExtend.height / 8, 1, &m_RayTracingSet);

	End(m_CommandBuffers[curentFrame]);


	//drawing
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	
	
	VkImage SwapChainImage = m_SwapChain.AcquireImage(m_ImageAvailableSemaphores[curentFrame]);
	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);

	if (SwapChainImage == VK_NULL_HANDLE)
	{
		m_SwapChain.Resize();

		for (auto& framebuffer : m_Framebuffers)
		{
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
		}

		m_ImageExtend = m_SwapChain.GetExtend();
		m_Framebuffers.resize(m_SwapChain.GetImageCount());

		for (size_t i = 0; i < m_SwapChain.GetImageCount(); i++)
		{
			VkImageView attachments[1] = { m_SwapChain.GetImageView(i) };
			VkFramebufferCreateInfo framebufferInfo =
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = m_GUIPass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = m_ImageExtend.width,
				.height = m_ImageExtend.height,
				.layers = 1
			};

			vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]);
		}

		return;
	}

	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	
	//copy pass
	VkCommandBufferBeginInfo beginBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(m_CommandBuffers[curentFrame], &beginBufferInfo);

	// 4. Prepare compute image for transfer
	VkImageMemoryBarrier prepareForTransferBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_ScreenImage.GetImageHandle(),
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	// 5. Transition swapchain image for receiving the copy
	VkImageMemoryBarrier prepareSwapchainBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = SwapChainImage,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	};

	// Combine both barriers
	VkImageMemoryBarrier barriers[] = { prepareForTransferBarrier, prepareSwapchainBarrier };

	vkCmdPipelineBarrier(m_CommandBuffers[curentFrame], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

	// 6. Execute the image copy operation
	VkExtent3D dstExtend = { m_ImageExtend.width, m_ImageExtend.height, 1 };
	Utilites::CmdBlitImage(m_CommandBuffers[curentFrame], m_ScreenImage.GetImageHandle(), m_ScreenImage.GetExtent(), SwapChainImage, dstExtend);

	// 7. Transition swapchain image to present layout
	Utilites::TransferImageDetails presentBarrier =
	{
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccess = VK_ACCESS_MEMORY_READ_BIT,
		.srcMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
		.dstMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
	};
	Utilites::CmdTransferImage(m_CommandBuffers[curentFrame], SwapChainImage, presentBarrier);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	End(m_CommandBuffers[curentFrame], waitStages, {m_ImageAvailableSemaphores[curentFrame]}, {m_RenderFinishedSemaphores[curentFrame]});

}

void Renderer::Present()
{
	VkSwapchainKHR swapchain = m_SwapChain.GetSwapChain();
	uint32_t index = m_SwapChain.GetActiveImageIndex();

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_RenderFinishedSemaphores[curentFrame],
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &index,
		.pResults = nullptr
	};

	VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		m_SwapChain.Resize();

		for (auto& framebuffer : m_Framebuffers)
		{
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
		}

		m_ImageExtend = m_SwapChain.GetExtend();
		m_Framebuffers.resize(m_SwapChain.GetImageCount());

		for (size_t i = 0; i < m_SwapChain.GetImageCount(); i++)
		{
			VkImageView attachments[1] = { m_SwapChain.GetImageView(i) };
			VkFramebufferCreateInfo framebufferInfo =
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = m_GUIPass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = m_ImageExtend.width,
				.height = m_ImageExtend.height,
				.layers = 1
			};

			vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]);
		}
	}

	curentFrame = (curentFrame + 1) % MAX_IN_FLIGHT_FRAMES;
}

void Renderer::UpdateCamera(const Camera& camera)
{
	s_RenderingUBO.model = glm::scale(glm::mat4(1.f), glm::vec3(0.10));
	s_RenderingUBO.view = camera.GetViewMatrix();
	s_RenderingUBO.proj = camera.GetProjectionMatrix();
	s_RenderingUBO.cameraPos = camera.GetPosition();

	m_UniformBuffers[curentFrame].Write(&s_RenderingUBO, sizeof(s_RenderingUBO));
}

void Renderer::UpdateRenderSetings(float stepSize)
{
	s_RenderingUBO.StepSize = stepSize;
}

void Renderer::RunSortStep(ComputePipeline& sortStep, uint32_t size, uint32_t algorithm)
{
	uint32_t data[] = { algorithm, size , s_SimUBO.particlesCount };

	uint32_t invocations = (s_SimUBO.particlesCount / size) * (size / 256);
	//invocations = s_SimUBO.particlesCount / 256;
	if (uint32_t rest = s_SimUBO.particlesCount % size)
	{
		uint32_t invocationsPerHeight = size / 256;
		uint32_t overflowInvocations = (size - rest) / 256;
		int32_t addedInvocations = invocationsPerHeight - 2 * overflowInvocations;
		invocations += std::max(addedInvocations, 0);
		invocations = std::min(s_SimUBO.particlesCount / 256, invocations);
	}

	//LOG(DEBUG, "Algorithm: %d, h:%d, inv:%d", algorithm, size, invocations);
	sortStep.Dispatch(m_CommandBuffers[curentFrame], invocations, 1, 1, &m_SortDescriptorSets[0], data);
}