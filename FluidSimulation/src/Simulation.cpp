#include "Simulation.h"
#include "Core/Input.h"
#include <glm/gtx/quaternion.hpp>

Simulation::Simulation(const ApplicationSpecification& spec)
	:Application(spec)
{
	m_Camera = CreateScope<Camera>(Camera::ProjectionType::Perspective);
	//m_Camera->SetViewportSize(32 + 2, 18 + 2);
}

Simulation::~Simulation()
{
}

void Simulation::OnEvent(Event& e)
{
	EventDispatcher dispacher(e);
	dispacher.Dispatch<WindowResizeEvent>(BIND_EVENT(Simulation::OnResize));

}

void Simulation::OnUpdate(float deltaTime)
{
	if (bPoused)
		return;

	UpdateCamera(deltaTime);

	m_Renderer->UpdateCamera(*m_Camera);
	m_Renderer->RenderFrame(deltaTime);
}

void Simulation::OnRender()
{
}

bool Simulation::OnResize(WindowResizeEvent& e)
{
	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		bPoused = true;
		return true;
	}

	bPoused = false;

	LOG(INFO, "Window size (%u x %u)", e.GetWidth(), e.GetHeight())

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
