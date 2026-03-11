#include "OrbitCameraController.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/Log.h"


#include "OrbitCameraController.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/Log.h"


void OrbitCameraController::Update(float dt, Input& input)
{
	assert(m_Camera && "Camera not bound to OrbitCameraController");

	glm::vec2 mouseDelta = input.GetMouseDelta();
	double mouseScroll = input.GetMouseScrollY();


	if (input.IsMousePressed(Mouse::Left))
	{
		glm::vec2 velocity = mouseDelta / dt * m_MouseSensitivity;
		// Smooth velocity
		m_AngularVelocity = glm::mix(m_AngularVelocity, velocity, 0.2f);
	}
	else
	{
		float decay = std::exp(-8.0f * dt);
		m_AngularVelocity *= decay;

		if (glm::length(m_AngularVelocity) < 0.001f)
			m_AngularVelocity = glm::vec2(0.0f);
	}



	// Rotate camera on left click
	m_Yaw   += m_AngularVelocity.x * dt;
	m_Pitch -= m_AngularVelocity.y * dt;


	// Zoom
	if (mouseScroll != 0.0 && !input.IsInUI())
	{
	}


	if (!input.IsInUI())
	{
		if (input.IsKeyPressed(Key::A) || input.IsKeyPressed(Key::Left))  m_Yaw   += m_KeySensitivity * dt;
		if (input.IsKeyPressed(Key::D) || input.IsKeyPressed(Key::Right)) m_Yaw   -= m_KeySensitivity * dt;
		if (input.IsKeyPressed(Key::W) || input.IsKeyPressed(Key::Up))    m_Pitch += m_KeySensitivity * dt;
		if (input.IsKeyPressed(Key::S) || input.IsKeyPressed(Key::Down))  m_Pitch -= m_KeySensitivity * dt;


		if (input.IsKeyPressedOnce(Key::Space))
		{
		}
		else if (input.IsKeyPressedOnce(Key::C))
		{
		}

		float decay = std::exp(-8.0f * dt);
		m_ZoomVelocity *= decay;

		if (std::abs(m_ZoomVelocity) < 0.001f)
			m_ZoomVelocity = 0.0f;

	}

	// Clamp radius
	if (m_Radius < m_MinRadius)
		m_Radius = m_MinRadius + 0.001f; // Small buffer
	if (m_Radius > m_MaxRadius)
		m_Radius = m_MaxRadius - 0.001f;

	// Prevent flipping
	// With euler angles cos(pitch) < 0 when pitch > 90, vice versa for pitch < -90
	// TODO: switch to quaternions
	m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

	// Spherical -> Cartesian
	glm::vec3 offset;
	offset.x = m_Radius * cos(glm::radians(m_Pitch)) * cos(glm::radians(m_Yaw));
	offset.y = m_Radius * sin(glm::radians(m_Pitch));
	offset.z = m_Radius * cos(glm::radians(m_Pitch)) * sin(glm::radians(m_Yaw));

	glm::vec3 position = m_Pivot + offset;

	m_Camera->SetPosition(position);
	m_Camera->SetRotation(-m_Pitch, m_Yaw + 180.0f);
	m_Camera->RecalculateView();

}


void OrbitCameraController::OnActivate(Input& input)
{
	// Recompute yaw/pitch from camera -> pivot direction
	glm::vec3 dir = glm::normalize(m_Camera->GetPosition() - m_Pivot);

	m_Yaw   = glm::degrees(atan2(dir.z, dir.x));
	m_Pitch = glm::degrees(asin(dir.y));

	// Lock radius to current distance
	m_Radius = glm::length(m_Camera->GetPosition() - m_Pivot);

	input.GetMouseDelta();
}

void OrbitCameraController::AddRadiusDelta(float d)
{
	m_Radius = std::max(0.1f, m_Radius + d);
}