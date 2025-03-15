#pragma once

#include "Core/Application.h"
#include "Events/WindowEvent.h"

class Simulation : public Application
{
public:
	Simulation(const ApplicationSpecification& spec);
	~Simulation();

	virtual void OnEvent(Event& e);
	virtual void OnRender() override;

private:

	bool OnResize(WindowResizeEvent& e);

private:
	bool bPoused = false;
};
