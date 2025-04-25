#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(ProjectionType projectionType)
	:m_ProjectionType(projectionType)
{
	RecalculateViewMatrix();
	RecalculatePerspectiveMatrix();
}

void Camera::SetViewportSize(uint32_t width, uint32_t height)
{
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	RecalculatePerspectiveMatrix();
}

const glm::vec3& Camera::GetRight()
{
	glm::vec3 RightVector = glm::cross(m_Forward, glm::vec3(0.0, 1.0, 0.0));
	RightVector = glm::normalize(RightVector);

	return RightVector;
}

void Camera::RecalculateViewMatrix()
{
	glm::vec3 RightVector = glm::cross(m_Forward, glm::vec3(0.0, 1.0, 0.0));
	RightVector = glm::normalize(RightVector);

	glm::vec3 UpWector = glm::cross(RightVector, m_Forward);
	UpWector = glm::normalize(UpWector);

	m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, UpWector);
}

void Camera::RecalculatePerspectiveMatrix()
{
	if (m_ProjectionType == ProjectionType::Perspective)
	{
		m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), (float)m_ViewportWidth, (float)m_ViewportHeight, m_NearClip, m_FarClip);
	}
	else
	{
		float orthoWidth = m_ViewportWidth / 2.f;
		float orthoHeight = m_ViewportHeight / 2.f;
		m_ProjectionMatrix = glm::orthoLH_ZO(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, m_NearClip, m_FarClip);
	}
}
