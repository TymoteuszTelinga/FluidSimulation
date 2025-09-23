#pragma once

#include "Core/Application.h"
#include "Events/WindowEvent.h"
#include "Events/KeyEvent.h"
#include "Renderer/Camera.h"

struct SpawnCube
{
	glm::vec3 center;
	glm::vec3 size;
};

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
	void AddObject(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 color, bool bCollision = false);
	void SpawnParticles();

	void GUI_ObjectList();
	void GUI_SpawnerList();
	void GUI_SimulationSpec();
	void GUI_RendererSpec();

	void SaveConfiguration(const std::string& path);
	void LoadConfiguration(const std::string& path);

private:
	bool bPoused = false;
	bool bMinimized = false;
	bool bSlowmo = false;

	Scope<Camera> m_Camera;
	glm::vec2 m_LastMousePosition;

	//Renderig

	//Fluid
	SimulationUBO m_ActiveFluid;
	UniformBufferObject m_ActiveRenderer;
	//Scene
	std::vector<Object> m_Objects;
	std::vector<SpawnCube> m_Spawners;
};
