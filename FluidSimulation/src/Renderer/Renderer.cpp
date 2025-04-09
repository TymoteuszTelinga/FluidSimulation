#include "Renderer.h"

#include <iostream>
#include <fstream>

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Utilities.h"

static struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 uv;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription description =
		{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX

		};
		return description;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAtributeDescription()
	{
		std::vector<VkVertexInputAttributeDescription> descriptions;

		//position attrib
		{
			VkVertexInputAttributeDescription VertexAttribute =
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, position)
			};

			descriptions.push_back(VertexAttribute);
		}

		//color attrib
		{
			VkVertexInputAttributeDescription VertexAttribute =
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, color)
			};

			descriptions.push_back(VertexAttribute);
		}

		//UV attrib
		{
			VkVertexInputAttributeDescription VertexAttribute =
			{
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, uv)
			};

			descriptions.push_back(VertexAttribute);
		}

		return descriptions;
	}
};

static struct Instance
{
	alignas(8) glm::vec2 position;
	alignas(4)	float density;
	alignas(8) glm::vec2 velocity;
	alignas(8) glm::vec2 predictedPosition;

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
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Instance, position)
			};

			descriptions.push_back(InstanceAttribute);
		}

		{
			VkVertexInputAttributeDescription InstanceAttribute =
			{
				.location = 4,
				.binding = 1,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Instance, velocity)
			};

			descriptions.push_back(InstanceAttribute);
		}


		return descriptions;
	}
};

static const float PS = 0.025f;
static const std::vector<Vertex> vertices = 
{
	{{-PS, -PS, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.f, 1.f}},
	{{ PS, -PS, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.f, 1.f}},
	{{ PS,  PS, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.f, 0.f}},
	{{-PS,  PS, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.f, 0.f}},
};

static const std::vector<uint16_t> indices = 
{
	0, 1, 2, 2, 3, 0
};

static std::vector<Instance> instances;

static struct UniformBufferObject
{
	alignas(16) glm::mat4 model = glm::mat4(1.f);
	alignas(16) glm::mat4 view  = glm::mat4(1.f);
	alignas(16) glm::mat4 proj  = glm::mat4(1.f);
} s_RenderingUBO;

static struct SimulationUBO
{
	float simulationHeight = 4.5f / 2.f * 4.f;//in use
	float simulationWidth = 8.f / 2.f * 4.f;//in use
	float collisionDamping = 0.8f;//in use
	float kernelRadius = 0.3;//in use
	float targetDensity = 2.75f;//in use
	float pressureMultiplier = 10.5f;//in use
	uint32_t particlesCount = 256 * 128;//in use (256 * 2^n)
} s_SimUBO;

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_Device);

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
	m_GraphicsSetLayout.Free();
	m_SimulationSetLayout.Free();
	m_SortSetLayout.Free();
	//vkDestroyDescriptorSetLayout(m_Device, m_SortDescriptorSetLayout, nullptr);

	m_VertexBuffer.Free();
	m_IndexBuffer.Free();
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

	m_SortStep.Free();
	m_OffsetStep.Free();

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);

	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

	ClearSwapChain();

	m_MSAAImage.Free();
	m_DepthImage.Free();

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
	/*float x = -7.0f;
	float y = -4.0f;*/
	s_RenderingUBO.proj = glm::orthoLH_ZO(-s_SimUBO.simulationWidth - 1.f, s_SimUBO.simulationWidth + 1.f, -s_SimUBO.simulationHeight - 1.f, s_SimUBO.simulationHeight + 1.f, 0.0f, 1.f);
	for (size_t i = 0; i < s_SimUBO.particlesCount; i++)
	{
		float x = (float)rand() / RAND_MAX;
		float y = (float)rand() / RAND_MAX;
		x = x * 2.f - 1.f;
		y = y * 2.f - 1.f;
		x *= s_SimUBO.simulationWidth;
		y *= s_SimUBO.simulationHeight;
		//y = 0.f;
		/*x += 0.1f;
		if (x >= 7.0f)
		{
			x = -7.0f;
			x += 0.1f;
			y += 0.1f;
		}*/



		Instance particle;
		particle.position = glm::vec2(x, y);
		particle.predictedPosition = particle.position;
		particle.density = 0.0f;
		particle.velocity = glm::vec2(0.f,0.f);
		instances.push_back(particle);
	}


	CreateInstance(applicationName);
	CreateDebugCallback();

	CreateSurface(window);
	PickPhysicalDevice();
	CreateLogicalDevice();

	CreateSwapChain();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateComputePipeline();

	CreateComandPool();

	ImageSpecification MSAASpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = m_ImageExtend,
		.format = c_SwapChainFormat.format,
		.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
		.samples = c_MSAACount,
		.tiling = VK_IMAGE_TILING_OPTIMAL
	};
	m_MSAAImage.Init(MSAASpec);

	ImageSpecification depthSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = m_ImageExtend,
		.format = VK_FORMAT_D32_SFLOAT,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
		.samples = c_MSAACount,
		.tiling = VK_IMAGE_TILING_OPTIMAL
	};
	m_DepthImage.Init(depthSpec);

	CreateFramebuffers();

	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateInstanceBuffer();
	CreateUniformBuffers();

	CreateDescriptorPool();

	CreateDescriptorSets();
	CreateComputeDescriptorSets();
	CreateSortDescriptorSets();

	CreateCommandBuffer();
	CreateSyncObjects();

	LOG(DEBUG, "Active Particles: %u", s_SimUBO.particlesCount);
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

void Renderer::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

	std::cout << "Devices deteted: " << deviceCount << '\n';
	for (auto& deviceHandle : devices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(deviceHandle, &deviceProperties);
		std::cout << "Device name: " << deviceProperties.deviceName << '\n';
		printf("\tAPI version: %d.%d.%d.%d\n", VK_API_VERSION_VARIANT(deviceProperties.apiVersion), VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			m_PhysicalDevice.PhysicalDeviceHandle = deviceHandle;

			uint32_t QFamyliesCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &QFamyliesCount, nullptr);

			m_PhysicalDevice.QueuesFalimies.resize(QFamyliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &QFamyliesCount, m_PhysicalDevice.QueuesFalimies.data());


			for (auto& FamilyProperiy : m_PhysicalDevice.QueuesFalimies)
			{
				printf("\tNum queues: %d ", FamilyProperiy.queueCount);

				VkQueueFlags Flags = FamilyProperiy.queueFlags;
				printf("\tGFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
					(Flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
					(Flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");
			}
			printf("\n");
		}

	}
}

void Renderer::CreateLogicalDevice()
{
	float queuePriority[] = {1.0f};

	uint32_t baseQueue = m_PhysicalDevice.GetFamilyQueue(VK_QUEUE_GRAPHICS_BIT);
	uint32_t presetQueue = m_PhysicalDevice.GetPresentQueue(m_Surface);

	if (baseQueue == presetQueue)
	{
		LOG(INFO, "one que to rule all");
	}

	VkDeviceQueueCreateInfo queueCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = m_PhysicalDevice.GetFamilyQueue(VK_QUEUE_GRAPHICS_BIT),
		.queueCount = 1,
		.pQueuePriorities = &queuePriority[0]
	};

	std::vector<const char*> DevExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
	};

	VkPhysicalDeviceFeatures deviceFeatures = { 0 };

	VkDeviceCreateInfo deviceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)DevExtensions.size(),
		.ppEnabledExtensionNames = DevExtensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	vkCreateDevice(m_PhysicalDevice.PhysicalDeviceHandle, &deviceCreateInfo, nullptr, &m_Device);

	vkGetDeviceQueue(m_Device, queueCreateInfo.queueFamilyIndex, 0, &m_ComputeQueue);
	vkGetDeviceQueue(m_Device, queueCreateInfo.queueFamilyIndex, 0, &m_Queue);
	vkGetDeviceQueue(m_Device, presetQueue, 0, &m_PresentQueue);

}

void Renderer::CreateSwapChain()
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice.PhysicalDeviceHandle, m_Surface, &SurfaceCapabilities);

	uint32_t ImagesCount = SurfaceCapabilities.minImageCount + 1;
	if (SurfaceCapabilities.maxImageCount > 0 && ImagesCount > SurfaceCapabilities.maxImageCount)
	{
		ImagesCount = SurfaceCapabilities.maxImageCount;
	}

	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;//alway suported (vsync on) in future chenge to Imidient mode (vsync off)
	PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; //vsync off

	uint32_t queueFamily = m_PhysicalDevice.GetFamilyQueue(VK_QUEUE_GRAPHICS_BIT);
	m_ImageExtend = SurfaceCapabilities.currentExtent;

	LOG(WARNING, "SwapChain resize %u | %u", m_ImageExtend.width, m_ImageExtend.height);

	VkSwapchainCreateInfoKHR SwapChainCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = m_Surface,
		.minImageCount = ImagesCount,
		.imageFormat = c_SwapChainFormat.format,
		.imageColorSpace = c_SwapChainFormat.colorSpace,
		.imageExtent = m_ImageExtend,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queueFamily,
		.preTransform = SurfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = PresentMode,
		.clipped = VK_TRUE
	};

	vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_Swapchain);

	m_Images.resize(ImagesCount);
	vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &ImagesCount, m_Images.data());
	m_Images.resize(ImagesCount);
	m_ImageViews.resize(ImagesCount);
	
	for (size_t i = 0; i < m_Images.size(); i++)
	{
		m_ImageViews[i] = Image::CreateImageView(m_Device, m_Images[i], c_SwapChainFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

}

void Renderer::CreateRenderPass()
{
	//msaa color attachment
	VkAttachmentDescription colorAttachment =
	{
		.flags = 0,
		.format = c_SwapChainFormat.format,
		.samples = c_MSAACount,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment =
	{
		.flags = 0,
		.format = VK_FORMAT_D32_SFLOAT,//get depth orrmat from image
		.samples = c_MSAACount,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmnetRef =
	{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	//non mssaa color attachment
	VkAttachmentDescription colorAttachmentResolve =
	{
		.flags = 0,
		.format = c_SwapChainFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentResolveRef =
	{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = &colorAttachmentResolveRef,
		.pDepthStencilAttachment = &depthAttachmnetRef
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

	VkAttachmentDescription attachments[3] = {colorAttachment, depthAttachment, colorAttachmentResolve};

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 3,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
}

void Renderer::CreateDescriptorSetLayout()
{
	//Graphics
	LayoutSpecification graphicsLayoutSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}
		//uniform
	};
	m_GraphicsSetLayout.Init(graphicsLayoutSpec);

	//Simulation
	LayoutSpecification simulationLayoutSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}
		//uniform, particles, spacial, lookup
	};
	m_SimulationSetLayout.Init(simulationLayoutSpec);

	//sorting
	LayoutSpecification sortLayoutSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.types = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}
		//spatial, lookup
	};
	m_SortSetLayout.Init(sortLayoutSpec);

}

void Renderer::CreateGraphicsPipeline()
{
	//Sahder creation
	VkShaderModule vertShaderModule = Utilites::CreateShaderModule(m_Device, "src\\Shaders\\vert.spv");
	VkShaderModule fragShaderModule = Utilites::CreateShaderModule(m_Device, "src\\Shaders\\frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//PipeLineDefinicion
	VkVertexInputBindingDescription VertexBinding[] = {Vertex::GetBindingDescription(), Instance::GetBindingDescription()};
	auto VertexAttributes = Vertex::GetAtributeDescription();
	auto InstanceAttributes = Instance::GetAtributeDescription();
	VertexAttributes.insert(VertexAttributes.end(), InstanceAttributes.begin(), InstanceAttributes.end());

	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 2,
		.pVertexBindingDescriptions = VertexBinding,
		.vertexAttributeDescriptionCount = (uint32_t)VertexAttributes.size(),
		.pVertexAttributeDescriptions = VertexAttributes.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicStateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = (uint32_t)dynamicStates.size(),
		.pDynamicStates = dynamicStates.data()
	};

	//potencialy to delete set up at runtime
	VkViewport viewport =
	{
		.x = 0.f,
		.y = 0.f,
		.width = (float)m_ImageExtend.width,
		.height = (float)m_ImageExtend.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	//potencialy to delete set up at runtime
	//stencil???
	VkRect2D scissor = 
	{
		.offset = { 0, 0 },
		.extent = m_ImageExtend
	};

	VkPipelineViewportStateCreateInfo viewportStateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizationInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,//VK_CULL_MODE_BACK_BIT
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.f,
		.depthBiasClamp = 0.f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.f,
	};

	VkPipelineMultisampleStateCreateInfo mulitsampleInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = c_MSAACount,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	//per frame buffer attachment
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, //VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, //VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f}
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f
	};

	//uniforms buffer?
	VkDescriptorSetLayout graphicsSetLayout = m_GraphicsSetLayout.GetHandle();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1, 
		//.pSetLayouts = &m_DescriptorSetLayout,
		.pSetLayouts = &graphicsSetLayout,
		.pushConstantRangeCount = 0, //edit
		.pPushConstantRanges = nullptr //edit
	};

	vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateInfo, //it is dynamic
		.pRasterizationState = &rasterizationInfo,
		.pMultisampleState = &mulitsampleInfo,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlendInfo,
		.pDynamicState = &dynamicStateInfo,
		.layout = m_PipelineLayout,
		.renderPass = m_RenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_GraphicsPipeline);

	vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
}

void Renderer::CreateComputePipeline()
{
	ComputePipelineSpecification gravitySpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "ExternalForce",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_GravityStep.Init(gravitySpec);
	
	ComputePipelineSpecification densitySpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "DensityStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_DensityStep.Init(densitySpec);

	ComputePipelineSpecification pressureSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "PressureStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_PressureStep.Init(pressureSpec);

	ComputePipelineSpecification viscositySpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "ViscosityStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_ViscosityStep.Init(viscositySpec);

	ComputePipelineSpecification updateSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Simulation\\simulation2D.spv",
		.entryPoint = "UpdateStep",
		.descriptorLayout = m_SimulationSetLayout.GetHandle(),
		.constantsSize = sizeof(float)
	};
	m_UpdateStep.Init(updateSpec);

	ComputePipelineSpecification sortSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Sorting\\Sort.spv",
		.entryPoint = "BitonicSort",
		.descriptorLayout = m_SortSetLayout.GetHandle(),
		.constantsSize = sizeof(uint32_t) * 2
	};
	m_SortStep.Init(sortSpec);

	ComputePipelineSpecification offsetSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.shaderPath = "src\\Shaders\\Sorting\\Sort.spv",
		.entryPoint = "CalculateOffsets",
		.descriptorLayout = m_SortSetLayout.GetHandle(),
		.constantsSize = 0
	};
	m_OffsetStep.Init(offsetSpec);
}

void Renderer::CreateFramebuffers()
{
	m_Framebuffers.resize(m_ImageViews.size());

	for (size_t i = 0; i < m_ImageViews.size(); i++)
	{
		VkImageView attachments[3] = { m_MSAAImage.GetImageView(), m_DepthImage.GetImageView(), m_ImageViews[i]};
		VkFramebufferCreateInfo framebufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = m_RenderPass,
			.attachmentCount = 3,
			.pAttachments = attachments,
			.width = m_ImageExtend.width,
			.height = m_ImageExtend.height,
			.layers = 1
		};

		vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]);
	}
}

void Renderer::CreateComandPool()
{
	VkCommandPoolCreateInfo commandPoolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_PhysicalDevice.GetFamilyQueue(VK_QUEUE_GRAPHICS_BIT)
	};

	vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_CommandPool);
}

void Renderer::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	BufferSpecification vertexSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_VertexBuffer.Init(vertexSpec, nullptr);

	BufferSpecification stagingSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer stagingBuffer;
	stagingBuffer.Init(stagingSpec, vertices.data());

	CopyBuffer(stagingBuffer.GetHandle(), m_VertexBuffer.GetHandle(), bufferSize);

	stagingBuffer.Free();
}

void Renderer::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

	BufferSpecification indexSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_IndexBuffer.Init(indexSpec, nullptr);

	BufferSpecification stagingSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer stagingBuffer;
	stagingBuffer.Init(stagingSpec, indices.data());

	CopyBuffer(stagingBuffer.GetHandle(), m_IndexBuffer.GetHandle(), bufferSize);

	stagingBuffer.Free();
}

void Renderer::CreateInstanceBuffer()
{
	VkDeviceSize bufferSize = sizeof(Instance) * instances.size();

	BufferSpecification InstanceSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_InstanceBuffer.Init(InstanceSpec, nullptr);

	BufferSpecification stagingSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
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
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
		.size = sizeof(uint32_t) * instances.size() * 2,//key, Particle index
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	m_SpatialBuffer.Init(SpatialSpec, nullptr);

	BufferSpecification LookupSpec =
	{
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
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
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
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
		.device = {m_Device, m_PhysicalDevice.PhysicalDeviceHandle},
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
	VkDescriptorPoolSize poolSizes[2] =
	{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = MAX_IN_FLIGHT_FRAMES
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = MAX_IN_FLIGHT_FRAMES
		}
	};

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = MAX_IN_FLIGHT_FRAMES * 3,
		.poolSizeCount = 2,
		.pPoolSizes = poolSizes,
	};

	vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);
}

void Renderer::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_IN_FLIGHT_FRAMES, m_GraphicsSetLayout.GetHandle());

	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_DescriptorPool,
		.descriptorSetCount = MAX_IN_FLIGHT_FRAMES,
		.pSetLayouts = layouts.data()
	};

	m_DescriptorSets.resize(MAX_IN_FLIGHT_FRAMES);
	vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data());

	for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++)
	{
		VkDescriptorBufferInfo bufferInfo =
		{
			.buffer = m_UniformBuffers[i].GetHandle(),
			.offset = 0,
			.range = m_UniformBuffers[i].GetSize()
		};

		VkWriteDescriptorSet descriptorWrite =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = m_DescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = nullptr
		};

		vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
	}

}

void Renderer::CreateComputeDescriptorSets()
{
	m_ComputeDescriptorSets = m_SimulationSetLayout.CreateSets(m_DescriptorPool, MAX_IN_FLIGHT_FRAMES, { m_SimulationSpecBuffer, m_InstanceBuffer, m_SpatialBuffer, m_LookUpBuffer });
}

void Renderer::CreateSortDescriptorSets()
{
	m_SortDescriptorSets = m_SortSetLayout.CreateSets(m_DescriptorPool, MAX_IN_FLIGHT_FRAMES, { m_SpatialBuffer, m_LookUpBuffer });
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

void Renderer::ClearSwapChain()
{
	for (auto& framebuffer : m_Framebuffers)
	{
		vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
	}

	for (auto& imageView : m_ImageViews)
	{
		vkDestroyImageView(m_Device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

static uint32_t curentFrame = 0;
void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(commandBuffer, &beginBufferInfo);

	VkRect2D renderArea = { {0,0}, m_ImageExtend };
	VkClearValue clearValues[2] = { {0.f, 0.f, 0.005f, 0.f}, {1.f, 0.f}};

	VkRenderPassBeginInfo beginRenderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_RenderPass,
		.framebuffer = m_Framebuffers[imageIndex],
		.renderArea = renderArea,
		.clearValueCount = 2,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(commandBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

	VkDeviceSize offset[2] = { 0,0 };
	VkBuffer vertexBuffers[2] = {m_VertexBuffer.GetHandle(), m_InstanceBuffer.GetHandle()};
	vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offset);

	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT16);

	//set up in pipeline as dynamic
	VkViewport viewport = 
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)m_ImageExtend.width,
		.height = (float)m_ImageExtend.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	//set up in pipeline as dynamic
	VkRect2D scissor = 
	{
		.offset = { 0, 0 },
		.extent = m_ImageExtend
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[curentFrame], 0, nullptr);

	//Draw
	//vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, indices.size(), s_SimUBO.particlesCount, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	vkEndCommandBuffer(commandBuffer);
}

void Renderer::RenderFrame(float deltaTime)
{
	//compute faze
	deltaTime = std::min(deltaTime, 1.f / 60.f);
	RunSimulationStep(m_GravityStep, deltaTime);

	//sorting
	uint32_t h = 256;
	RunSortStep(m_SortStep, h, 0);//local BMS
	h *= 2;
	for (; h <= s_SimUBO.particlesCount; h *= 2) 
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
				RunSortStep(m_SortStep, hh, 2);
			}
		}
	}

	//ofsets
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);

	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	m_OffsetStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 128, &m_SortDescriptorSets[0]);

	VkSubmitInfo computeSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffers[curentFrame],
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};
	vkQueueSubmit(m_ComputeQueue, 1, &computeSubmitInfo, m_InFlightFences[curentFrame]);
	//offsets
	RunSimulationStep(m_DensityStep, deltaTime);
	RunSimulationStep(m_PressureStep, deltaTime);
	RunSimulationStep(m_ViscosityStep, deltaTime);
	RunSimulationStep(m_UpdateStep, deltaTime);
	

	//drawing
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[curentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
		return;
	}

	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);
	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	recordCommandBuffer(m_CommandBuffers[curentFrame], imageIndex);

	//updateUniformBuffer(deltaTime, curentFrame);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_ImageAvailableSemaphores[curentFrame],
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffers[curentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &m_RenderFinishedSemaphores[curentFrame]
	};

	vkQueueSubmit(m_Queue, 1, &submitInfo, m_InFlightFences[curentFrame]);

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_RenderFinishedSemaphores[curentFrame],
		.swapchainCount = 1,
		.pSwapchains = &m_Swapchain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr
	};

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}

	curentFrame = (curentFrame + 1) % MAX_IN_FLIGHT_FRAMES;
}

void Renderer::updateUniformBuffer(float deltaTime, uint32_t currentImage)
{
	UniformBufferObject ubo;
	ubo.model = glm::scale(glm::mat4(1.f), glm::vec3(1.0f));
	//ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(90.f) * deltaTime, glm::vec3(0.f, 0.f, 1.f));
	ubo.view = glm::mat4(1.f);
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
	//float aspectRacio = (float)m_ImageExtend.width / (float)m_ImageExtend.height;
	//ubo.proj = glm::perspective(glm::radians(45.f), aspectRacio, 0.1f, 10.f);

	ubo.proj = glm::orthoLH_ZO(-s_SimUBO.simulationWidth -1.f, s_SimUBO.simulationWidth +1.f, -s_SimUBO.simulationHeight -1.f, s_SimUBO.simulationHeight +1.f, 0.0f, 1.f);

	m_UniformBuffers[currentImage].Write(&ubo, sizeof(ubo));
}

void Renderer::RunSimulationStep(ComputePipeline& siulationStep, float deltaTime)
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);

	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	siulationStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, &m_ComputeDescriptorSets[0], &deltaTime);

	VkSubmitInfo computeSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffers[curentFrame],
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};
	vkQueueSubmit(m_ComputeQueue, 1, &computeSubmitInfo, m_InFlightFences[curentFrame]);
}

void Renderer::RunSortStep(ComputePipeline& sortStep, uint32_t size, uint32_t algorithm)
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);

	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	uint32_t data[] = { algorithm, size };
	sortStep.Dispatch(m_CommandBuffers[curentFrame], s_SimUBO.particlesCount / 256, &m_SortDescriptorSets[0], data);

	VkSubmitInfo computeSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffers[curentFrame],
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};
	vkQueueSubmit(m_ComputeQueue, 1, &computeSubmitInfo, m_InFlightFences[curentFrame]);
}

void Renderer::RecreateSwapChain()
{
	vkDeviceWaitIdle(m_Device);

	ClearSwapChain();

	CreateSwapChain();

	m_MSAAImage.Resize(m_ImageExtend);
	m_DepthImage.Resize(m_ImageExtend);

	CreateFramebuffers();
}

uint32_t Renderer::PhysicalDevice::GetFamilyQueue(VkQueueFlags queueType)
{
	for (size_t i = 0; i < QueuesFalimies.size(); i++)
	{
		if (QueuesFalimies[i].queueFlags & queueType)
		{
			return i;
		}
	}
	return 0;
}

uint32_t Renderer::PhysicalDevice::GetPresentQueue(VkSurfaceKHR surface)
{
	for (size_t i = 0; i < QueuesFalimies.size(); i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDeviceHandle, i, surface, &presentSupport);
		if (presentSupport)
		{
			return i;
		}
	}
	return 0;
}
