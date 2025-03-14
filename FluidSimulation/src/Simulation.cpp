#include "Simulation.h"

Simulation::Simulation(const ApplicationSpecification& spec)
	:Application(spec)
{
}

Simulation::~Simulation()
{
}

void Simulation::OnRender()
{
	m_Renderer->RenderFrame();
}
