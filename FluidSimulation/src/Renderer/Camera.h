#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	enum class ProjectionType { Perspective = 0, Orthographic = 1 };

public:
	Camera(ProjectionType projectionType = ProjectionType::Orthographic);

	void SetViewportSize(uint32_t width, uint32_t height);

	const glm::vec3& GetPosition() const { return m_Position; }
	void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }

	const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

	const glm::vec3& GetForward() { return m_Forward; }
	void SetForward(const glm::vec3& forward) { m_Forward = forward; RecalculateViewMatrix(); }
	const glm::vec3& GetRight();

private:
	void RecalculateViewMatrix();
	void RecalculatePerspectiveMatrix();

private:
	ProjectionType m_ProjectionType;
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.f);
	glm::mat4 m_ViewMatrix = glm::mat4(1.f);

	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_Forward = { 0.0f, 0.0f, -1.0f };

	uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
	float m_NearClip = 0.1f;
	float m_FarClip = 1000.f;

	float m_AspectRatio = 0.0f;
};