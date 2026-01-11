#pragma once
#include "Camera.h"


class PerspectiveCamera : public Camera
{

public:
	PerspectiveCamera(float fov, float aspect, float nearPlane, float farPlane);

	void SetPosition(const glm::vec3& pos)   { m_Position = pos; GetViewMatrix(); }
	void SetRotation(float pitch, float yaw) { m_Pitch = pitch; m_Yaw = yaw; GetViewMatrix(); }
	void SetAspect(float aspect)			 { m_Aspect = aspect; }

	const float GetYaw()           const { return m_Yaw; }
	const float GetPitch()         const { return m_Pitch; }

	const glm::mat4& GetViewMatrix()	   const override { return m_ViewMatrix; }
	const glm::mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
	const glm::vec3& GetPosition()		   const override { return m_Position; }
	glm::vec3 GetForward() const override;

	void RecalculateView();
	void RecalculateProjection();
private:
	float m_FOV, m_Aspect, m_Near, m_Far;
	float m_Pitch = 0.0f, m_Yaw = -90.0f;
	float xInitPos = 0.0f;
	float yInitPos = 0.0f;
	float zInitPos = 3.0f;
	glm::vec3 m_Position{0.0f};

	glm::mat4 m_ViewMatrix{1.0f};
	glm::mat4 m_ProjectionMatrix{1.0f};
};