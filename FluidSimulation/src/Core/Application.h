#pragma once

#include "Window.h"
#include "Base.h"
#include <string>
#include "Renderer/Renderer.h"

struct ApplicationSpecification
{
	std::string Name = "Application";
	uint32_t Width = 1280;
	uint32_t Height = 720;
};

class Application
{
public:
	Application(const ApplicationSpecification& spec);
	~Application();

	virtual void Run();

	virtual void OnEvent(Event& e) {};
	virtual void OnRender() {};

private:
	void OnEventApp(Event& e);

protected:
	ApplicationSpecification m_Specification;
	Scope<Renderer> m_Renderer;

private:
	Scope<Window> m_Window;

	bool m_Runing = false;
	float m_DeltaTime = 0.0f;
	float m_LastTime = 0.0f;

private:
	static Application* s_Instance;

};