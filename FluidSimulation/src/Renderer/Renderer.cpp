#include "Renderer.h"

#include <iostream>
#include <fstream>

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

	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);

	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);

	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

	ClearSwapChain();

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
	CreateInstance(applicationName);
	CreateDebugCallback();
	CreateSurface(window);
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
		//CreateImageView();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateComandPool();
	CreateCommandBuffer();
	CreateSyncObjects();
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

static const std::string GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity)
{
	switch (Severity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "Verbose";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "Info";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "Warning";

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "Error";

	default:
		return "UnKnown";
	}

	return "NO SUCH SEVERITY!";
}

static const std::string GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type)
{
	switch (Type) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		return "General";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		return "Validation";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		return "Performance";

	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		return "Device address binding";

	default:
		return "UnKnown";
	}

	return "NO SUCH TYPE!";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	printf("Debug Callback:\n");
	printf("   Severity: %s\n", GetDebugSeverityStr(messageSeverity).c_str());
	printf("   Type: %s\n", GetDebugType(messageTypes).c_str());
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
	//PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; //vsync off

	//chek if phisical device suport specific format 
	m_SwapChainFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
	m_SwapChainFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

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
		.imageFormat = m_SwapChainFormat.format,
		.imageColorSpace = m_SwapChainFormat.colorSpace,
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
		m_ImageViews[i] = CreateImageView(m_Device, m_Images[i], m_SwapChainFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

}

void Renderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment =
	{
		.flags = 0,
		.format = m_SwapChainFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
		.pColorAttachments = &colorAttachmentRef
	};
	
	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
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

	vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
}

static std::vector<char> readFile(const std::string& filename) 
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		LOG(ERROR, "Open file Error (%s)", filename.c_str());
	}

	file.seekg(0, std::ios::end);
	uint32_t fileSize = (uint32_t)file.tellg();

	std::vector<char> buffer(fileSize);

	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code) 
{
	VkShaderModuleCreateInfo createInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};

	VkShaderModule shaderModule;
	vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	return shaderModule;
}

void Renderer::CreateGraphicsPipeline()
{
	//Sahder creation
	std::vector<char> vertShaderCode = readFile("src\\Shaders\\vert.spv");//shader error
	std::vector<char> fragShaderCode = readFile("src\\Shaders\\frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(m_Device, vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(m_Device, fragShaderCode);

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

	//PipeLIneDefinicion
	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 0, //edit
		.pVertexBindingDescriptions = nullptr, //edit
		.vertexAttributeDescriptionCount = 0, //edit
		.pVertexAttributeDescriptions = nullptr, //edit
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
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
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
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	//per frame buffer attachment
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
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

	//uniforms buffer?
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0, //edit
		.pSetLayouts = nullptr, //edit
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
		.pDepthStencilState = nullptr,
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

void Renderer::CreateFramebuffers()
{
	m_Framebuffers.resize(m_ImageViews.size());

	for (size_t i = 0; i < m_ImageViews.size(); i++)
	{
		VkFramebufferCreateInfo framebufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = m_RenderPass,
			.attachmentCount = 1,
			.pAttachments = &m_ImageViews[i],
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

VkImageView Renderer::CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags)
{
	VkImageViewCreateInfo ViewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = Image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = Format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = {
			.aspectMask = AspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	VkImageView ImageView;
	vkCreateImageView(Device, &ViewInfo, NULL, &ImageView);
	return ImageView;
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
	VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };

	VkRenderPassBeginInfo beginRenderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_RenderPass,
		.framebuffer = m_Framebuffers[imageIndex],
		.renderArea = renderArea,
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};

	vkCmdBeginRenderPass(commandBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

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

	//Draw
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	vkEndCommandBuffer(commandBuffer);
}

static uint32_t curentFrame = 0;
void Renderer::RenderFrame()
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[curentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[curentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{

		RecreateSwapChain();
		return;
	}

	vkResetFences(m_Device, 1, &m_InFlightFences[curentFrame]);
	vkResetCommandBuffer(m_CommandBuffers[curentFrame], 0);
	recordCommandBuffer(m_CommandBuffers[curentFrame], imageIndex);

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
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
	}

	curentFrame = (curentFrame + 1) % MAX_IN_FLIGHT_FRAMES;
}

void Renderer::RecreateSwapChain()
{
	vkDeviceWaitIdle(m_Device);

	ClearSwapChain();

	CreateSwapChain();
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
