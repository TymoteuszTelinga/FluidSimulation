#include "Application.h"

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

	LOG(INFO, "Aplication Init complite");
}

Application::~Application()
{
}

void Application::Run()
{
	m_Runing = true;

	uint64_t counter = 0;
	float TotalTime = 0.f;


	//main loop
	while (m_Window->IsOpen() && m_Runing)
	{
		/*counter++;
		TotalTime += m_DeltaTime;

		if (TotalTime >= 1.0 / 10.0)
		{
			float FPS = (1.0 / TotalTime) * counter;
			float FrameTime = TotalTime / counter * 1000;

			printf("FPS: %.1f\n", FPS);
			printf("Time: %.3f\n", FrameTime);

			TotalTime = 0.0f;
			counter = 0;
		}*/

		m_Window->OnUpdate();

		OnUpdate(m_DeltaTime);
		OnRender();

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
