#include "Simulation.h"
#include "Core/Input.h"
#include <glm/gtx/quaternion.hpp>
#include <imgui/imgui.h>

Simulation::Simulation(const ApplicationSpecification& spec)
	:Application(spec)
{
	m_Camera = CreateScope<Camera>(Camera::ProjectionType::Perspective);
	//m_Camera->SetViewportSize(32 + 2, (4.5f * 6.f) + 2);
	m_Camera->SetPosition(glm::vec3(0.f, 0.f, -40.f));
}

Simulation::~Simulation()
{
}

void Simulation::OnEvent(Event& e)
{
	EventDispatcher dispacher(e);
	dispacher.Dispatch<WindowResizeEvent>(BIND_EVENT(Simulation::OnResize));
	dispacher.Dispatch<KeyPressedEvent>(BIND_EVENT(Simulation::OnKeyPressed));
}

void Simulation::OnUpdate(float deltaTime)
{
	if (bMinimized)
		return;

	UpdateCamera(deltaTime);

	m_Renderer->UpdateCamera(*m_Camera);
	if (!bPoused)
	{
		m_Renderer->SimulationStep(bSlowmo? deltaTime*0.2f : deltaTime);
	}
	else if (Input::IsKeyDown(GLFW_KEY_RIGHT))
	{
		m_Renderer->SimulationStep(deltaTime);
	}
}

void Simulation::OnRender()
{
	if (bMinimized)
		return;

	m_Renderer->RenderFrame(0.1f);
}

void Simulation::OnGUIRender()
{
	ImGui::Begin("Controls");
	ImGui::Checkbox("Slowmotion", &bSlowmo);
	if (ImGui::Button(bPoused ? "Play" : "Stop"))
	{
		bPoused = !bPoused;
	}


	ImGui::SeparatorText("Camera");

	glm::vec3 CameraPos = m_Camera->GetPosition();
	if (ImGui::DragFloat3("Camera position", &CameraPos.x))
	{
		LOG(DEBUG, "Camera Update");
		m_Camera->SetPosition(CameraPos);
	}

	glm::vec3 CameraForward = m_Camera->GetForward();
	if (ImGui::DragFloat3("Camera forward", &CameraForward.x))
	{
		m_Camera->SetForward(CameraForward);
	}

	float FOV = m_Camera->GetFOV();
	if (ImGui::DragFloat("FOV", &FOV))
	{
		FOV = std::max(1.f, FOV);
		m_Camera->SetFOV(FOV);
	}

	ImGui::SeparatorText("Rendering");
	static float stepSize = 0.02f;
	if (ImGui::DragFloat("StepSize", &stepSize, 0.001f, 0.001f, 0.1f))
	{
		m_Renderer->UpdateRenderSetings(stepSize);
	}

	ImGui::End();
}

bool Simulation::OnResize(WindowResizeEvent& e)
{
	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		bMinimized = true;
		return true;
	}

	bMinimized = false;

	LOG(INFO, "Window size (%u x %u)", e.GetWidth(), e.GetHeight())

	return true;
}

bool Simulation::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == GLFW_KEY_SPACE && !e.IsReapeted())
	{
		bPoused = !bPoused;
		LOG(DEBUG, "Simulation state: %s", bPoused ? "Poused" : "played");
	}

	if (e.GetKeyCode() == GLFW_KEY_R)
	{
		m_Renderer->RestartSimulation();
		bPoused = true;
	}

	if (e.GetKeyCode() == GLFW_KEY_Z)
	{
		bSlowmo = !bSlowmo;
		LOG(DEBUG, "Slowmo state: %s", bSlowmo ? "Active" : "Disable");
	}

	return true;
}

void Simulation::UpdateCamera(float deltaTime)
{
	glm::vec2 MousePos = Input::GetMousePosition();
	glm::vec2 delta = (MousePos - m_LastMousePosition) * 0.02f;
	m_LastMousePosition = MousePos;

	if (!Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		Input::SetCursorMode(GLFW_CURSOR_NORMAL);
		return;
	}
	Input::SetCursorMode(GLFW_CURSOR_DISABLED);

	if (delta.x != 0.f || delta.y != 0.f)
	{
		float PitchDelta = -delta.y * 0.3;
		float YawDelta = delta.x * 0.3;

		glm::quat q = glm::normalize(glm::cross(glm::angleAxis(PitchDelta, m_Camera->GetRight()), glm::angleAxis(YawDelta, glm::vec3(0.f, 1.f, 0.f))));
		m_Camera->SetForward(glm::rotate(q, m_Camera->GetForward()));
	}

	float Speed = 10.f;
	bool bMoved = false;
	glm::vec3 CameraPos = m_Camera->GetPosition();

	//Frorward
	if (Input::IsKeyDown(GLFW_KEY_W))
	{
		CameraPos -= m_Camera->GetForward() * Speed * deltaTime;
		bMoved = true;
	}
	//backward
	if (Input::IsKeyDown(GLFW_KEY_S))
	{
		CameraPos += m_Camera->GetForward() * Speed * deltaTime;
		bMoved = true;
	}
	//Right
	if (Input::IsKeyDown(GLFW_KEY_D))
	{
		CameraPos += m_Camera->GetRight() * Speed * deltaTime;
		bMoved = true;
	}
	//Left
	if (Input::IsKeyDown(GLFW_KEY_A))
	{
		CameraPos -= m_Camera->GetRight() * Speed * deltaTime;
		bMoved = true;
	}
	//Up
	if (Input::IsKeyDown(GLFW_KEY_Q))
	{
		CameraPos -= glm::vec3(0.f, 1.f, 0.f) * Speed * deltaTime;
		bMoved = true;
	}
	//Down
	if (Input::IsKeyDown(GLFW_KEY_E))
	{
		CameraPos += glm::vec3(0.f, 1.f, 0.f) * Speed * deltaTime;
		bMoved = true;
	}

	if (bMoved)
	{
		m_Camera->SetPosition(CameraPos);
		//LOG(DEBUG, "camera position: %.2f, %.2f, %.2f", CameraPos.x, CameraPos.y, CameraPos.z);
	}
}

void Simulation::ToggleSlowmo()
{
	bSlowmo = !bSlowmo;
}
