#include "Simulation.h"

Simulation::Simulation(const ApplicationSpecification& spec)
	:Application(spec)
{
}

Simulation::~Simulation()
{
}

void Simulation::OnEvent(Event& e)
{
	EventDispatcher dispacher(e);
	dispacher.Dispatch<WindowResizeEvent>(BIND_EVENT(Simulation::OnResize));

}

void Simulation::OnRender()
{
	if (!bPoused)
	{
		m_Renderer->RenderFrame();
	}
}

bool Simulation::OnResize(WindowResizeEvent& e)
{
	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		bPoused = true;
		return true;
	}

	bPoused = false;


	return true;
}
