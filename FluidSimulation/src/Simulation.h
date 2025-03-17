#pragma once

#include "Core/Application.h"
#include "Events/WindowEvent.h"

class Simulation : public Application
{
public:
	Simulation(const ApplicationSpecification& spec);
	~Simulation();

	virtual void OnUpdate(float deltaTime) override;
	virtual void OnRender() override;
	virtual void OnEvent(Event& e);

private:

	bool OnResize(WindowResizeEvent& e);

private:
	bool bPoused = false;
};
