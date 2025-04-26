#pragma once

#include "Core/Application.h"
#include "Events/WindowEvent.h"
#include "Events/KeyEvent.h"
#include "Renderer/Camera.h"

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
	bool OnKeyPressed(KeyPressedEvent& e);
	void UpdateCamera(float deltaTime);

private:
	bool bPoused = true;
	bool bMinimized = false;

	Scope<Camera> m_Camera;
	glm::vec2 m_LastMousePosition;

};
