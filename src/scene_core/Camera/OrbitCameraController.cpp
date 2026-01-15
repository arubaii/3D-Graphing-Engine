#include "OrbitCameraController.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/Log.h"


void OrbitCameraController::Update(float dt, Input& input)
{

	assert(m_Camera && "Camera not bound to FreeCameraController");



	glm::vec2 mouse    = input.GetMouseDelta();
	double mouseScroll = input.GetMouseScrollY();

	const float mouseSensitivity     = 0.1f;
	const float mouseScrollSensivity = 1.0f;
	const float keySensitivity	     = 135.0f; // 135 deg per second
	const float keyZoomSensitivity   = 10.0f;
	const float minRadius            = 5.0f;
	const float maxRadius		     = 100000.0f;
	// TODO: Set min and max radius proportional to the size the pivot object

	if (input.IsMousePressed(Mouse::Left))
	{
		m_Yaw    += mouse.x * mouseSensitivity;
		m_Pitch  -= mouse.y * mouseSensitivity;
	}
	m_Radius -= mouseScroll; // Mouse Zoom

	if (input.IsKeyboardEnabled())
	{
		if (input.IsKeyPressed(Key::A) || input.IsKeyPressed(Key::Left))  m_Yaw   += keySensitivity * dt;
		if (input.IsKeyPressed(Key::D) || input.IsKeyPressed(Key::Right)) m_Yaw   -= keySensitivity * dt;
		if (input.IsKeyPressed(Key::W) || input.IsKeyPressed(Key::Up))    m_Pitch += keySensitivity * dt;
		if (input.IsKeyPressed(Key::S) || input.IsKeyPressed(Key::Down))  m_Pitch -= keySensitivity * dt;
		if (input.IsKeyPressed(Key::Space))    m_Radius -= keyZoomSensitivity * dt;
		if (input.IsKeyPressed(Key::LeftCtrl)) m_Radius += keyZoomSensitivity * dt;
	}

	// Clamp radius
	if (m_Radius < minRadius)
		m_Radius = minRadius + 0.001f; // Small buffer
	if (m_Radius > maxRadius)
		m_Radius = maxRadius - 0.001f;

	// Prevent flipping
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
