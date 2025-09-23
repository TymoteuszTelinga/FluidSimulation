#include "Application.h"

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "Log.h"

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& spec)
	:m_Specification(spec)
{
	s_Instance = this;

	m_Window = CreateScope<Window>(m_Specification.Width, m_Specification.Height);
	m_Window->SetEventCallback(BIND_EVENT(Application::OnEventApp));

	m_Renderer = CreateScope<Renderer>();
	m_Renderer->Init(m_Specification.Name, m_Window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	//setup ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 5.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	SetGUIColors();

	ImGui_ImplGlfw_InitForVulkan(m_Window->GetWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.ApiVersion	 = VK_API_VERSION_1_0;
	init_info.Instance		 = m_Renderer->GetInstance();
	init_info.PhysicalDevice = m_Renderer->GetDevice().physicalDevice;
	init_info.Device		 = m_Renderer->GetDevice().device;
	init_info.QueueFamily	 = m_Renderer->GetQueueFamily();
	init_info.Queue			 = m_Renderer->GetQueue();
	init_info.PipelineCache  = VK_NULL_HANDLE;
	init_info.DescriptorPool = m_Renderer->GetDescriptorPool();
	init_info.RenderPass	 = m_Renderer->GetRenderPass();
	init_info.Subpass		 = 0;
	init_info.MinImageCount	 = 2;
	init_info.ImageCount	 = 3; //TODO
	init_info.MSAASamples	 = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator		 = nullptr;
	init_info.CheckVkResultFn = nullptr;
	init_info.UseDynamicRendering = false;
	ImGui_ImplVulkan_Init(&init_info);
	/*
	*/

	LOG(INFO, "Aplication Init complite");
}

void RenderImGui(Scope<Renderer>& renderer)
{
	VkCommandBuffer commandBuffer = renderer->m_CommandBuffers[0];
	renderer->Begin(commandBuffer);

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = renderer->GetRenderPass();
	info.framebuffer = renderer->m_Framebuffers[renderer->m_SwapChain.GetActiveImageIndex()];
	info.renderArea.extent = renderer->m_ImageExtend;
	info.clearValueCount = 0;
	info.pClearValues = nullptr;
	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	renderer->End(commandBuffer, &waitStage);
}

Application::~Application()
{
	m_Renderer->Wait();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Application::Run()
{
	m_Runing = true;
	ImGuiIO& io = ImGui::GetIO();

	uint64_t counter = 0;
	float TotalTime = 0.f;


	//main loop
	while (m_Window->IsOpen() && m_Runing)
	{
		counter++;
		TotalTime += m_DeltaTime;

		if (TotalTime >= 1.0 / 1.0)
		{
			float FPS = (1.0 / TotalTime) * counter;
			float FrameTime = TotalTime / counter * 1000;

			LOG(INFO, "FPS: %.1f, Time: %.3f ms", FPS, FrameTime)
			#ifndef _DEBUG
				printf("FPS: %.1f, ", FPS);
				printf("Time: %.3f\n", FrameTime);
			#endif // _DEBUG

			TotalTime = 0.0f;
			counter = 0;
		}

		m_Window->OnUpdate();

		OnUpdate(m_DeltaTime);
		OnRender();

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		OnGUIRender();

		ImGui::Render();
		RenderImGui(m_Renderer);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			//glfwMakeContextCurrent(backup_current_context);
		}

		m_Renderer->Present();

		float time = glfwGetTime();
		m_DeltaTime = time - m_LastTime;
		m_LastTime = time;
	}
}

void Application::OnEventApp(Event& e)
{
	//Log Event
	OnEvent(e);
}

void Application::SetGUIColors()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);

	colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.38f, 0.38f, 0.38f, 0.80f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

	colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.38f, 0.38f, 0.38f, 0.80f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

	/*
	colors[ImGuiCol_CheckMark]              = ImVec4(0.98f, 0.93f, 0.26f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.98f, 0.93f, 0.26f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.00f, 0.95f, 0.27f, 1.00f);
	*/

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_PickerHueWheel);
}
