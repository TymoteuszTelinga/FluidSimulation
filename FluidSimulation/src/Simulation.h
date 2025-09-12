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
	virtual void OnGUIRender() override;
	virtual void OnEvent(Event& e);

private:

	bool OnResize(WindowResizeEvent& e);
	bool OnKeyPressed(KeyPressedEvent& e);
	void UpdateCamera(float deltaTime);
	void ToggleSlowmo();
	void AddObject(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 color);

private:
	bool bPoused = false;
	bool bMinimized = false;
	bool bSlowmo = false;

	Scope<Camera> m_Camera;
	glm::vec2 m_LastMousePosition;
	std::vector<Object> m_Objects;
};
