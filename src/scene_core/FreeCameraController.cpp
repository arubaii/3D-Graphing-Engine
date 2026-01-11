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
	glm::vec3 forward = m_Camera->GetForwardVector();
	glm::vec3 right   = m_Camera->GetRightVector();
	glm::vec3 up	  = m_Camera->GetUpVector();

	glm::vec3 position = m_Camera->GetPosition();

	// =========== Keyboard Movement ===========

	glm::vec3 move{0.0f};

	if (input.IsKeyPressed(Key::W)) 	   move += forward;
	if (input.IsKeyPressed(Key::S)) 	   move -= forward;
	if (input.IsKeyPressed(Key::A)) 	   move -= right;
	if (input.IsKeyPressed(Key::D))		   move += right;
	if (input.IsKeyPressed(Key::Space))    move += up;
	if (input.IsKeyPressed(Key::LeftCtrl)) move -= up;

	if (move != glm::vec3(0.0f))
		position += glm::normalize(move) * velocity;

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
