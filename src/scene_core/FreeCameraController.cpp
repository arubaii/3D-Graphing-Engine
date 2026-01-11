#include <algorithm>
#include "FreeCameraController.h"
#include "utils/Base.h"
// #include "utils/LOG.h"


void FreeCameraController::Update(float dt, Input& input)
{
	assert(m_Camera && "Camera not bound to FreeCameraController");

	if (input.IsCursorEnabled())
		return;

	const float velocity = m_MoveSpeed * dt;

	// Direction vectors
	glm::vec3 forward = m_Camera->GetForward();
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	glm::vec3 position = m_Camera->GetPosition();

	// =========== Keyboard Movement ===========

	if (input.IsKeyPressed(Key::W)) 		 position += forward * velocity;
	if (input.IsKeyPressed(Key::S)) 		 position -= forward * velocity;
	if (input.IsKeyPressed(Key::A)) 		 position -= right   * velocity;
	if (input.IsKeyPressed(Key::D)) 		 position += right   * velocity;
	if (input.IsKeyPressed(Key::Space))      position += up * velocity;
	if (input.IsKeyPressed(Key::LeftCtrl))   position -= up * velocity;

	m_Camera->SetPosition(position);
	// ============ Mouse Movement ============
	glm::vec2 mouse = input.GetMouseDelta();

	float yaw   = m_Camera->GetYaw();
	float pitch = m_Camera->GetPitch();

	yaw   += mouse.x * m_MouseSensitivity;
	pitch += mouse.y * m_MouseSensitivity;

	pitch = std::clamp(pitch, -89.0f, 89.0f);

	m_Camera->SetRotation(pitch, yaw);
	m_Camera->RecalculateView();

}

void FreeCameraController::OnActivate(Input& input)
{
	// Reset mouse state so first frame doesn't jump
	input.GetMouseDelta();
}
