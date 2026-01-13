#include "PerspectiveCamera.h"

PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float nearPlane, float farPlane)
	: m_FOV(fov), m_Aspect(aspect), m_Near(nearPlane), m_Far(farPlane)
{
	m_Position = { xInitPos, yInitPos, zInitPos };
	RecalculateProjection();
	RecalculateView();
}


void PerspectiveCamera::RecalculateView()
{
	glm::vec3 forward;
	forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	forward.y = sin(glm::radians(m_Pitch));
	forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

	forward = glm::normalize(forward);

	const glm::vec3 up = { 0.0f, 1.0f, 0.0f };

	m_ViewMatrix = glm::lookAt
	(
		m_Position,
		m_Position + forward,
		up
	);

};

void PerspectiveCamera::RecalculateProjection()
{
	m_ProjectionMatrix = glm::perspective(
		glm::radians(m_FOV),
		m_Aspect,
		m_Near,
		m_Far
	);

};

glm::vec3 PerspectiveCamera::GetForwardVector() const
{
	return glm::normalize(glm::vec3(
		cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)), // x
		sin(glm::radians(m_Pitch)),							   // y
		sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))  // z
	));
}

glm::vec3 PerspectiveCamera::GetRightVector() const
{
	return glm::normalize(glm::cross(GetForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 PerspectiveCamera::GetUpVector() const
{
	return glm::normalize(glm::cross(GetRightVector(), GetForwardVector()));
}