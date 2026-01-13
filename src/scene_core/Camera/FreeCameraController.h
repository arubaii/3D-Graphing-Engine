#pragma once
#include "CameraController.h"

class FreeCameraController : public CameraController
{
private:
	// PerspectiveCamera& m_Camera;
	float m_MoveSpeed = 25.0f;
	float m_MouseSensitivity = 0.1f;

	glm::vec3 GetFrontVector();
	glm::vec3 GetUpVector();

public:
	FreeCameraController() = default;
	void Update(float dt, Input& input) override;
	void OnActivate(Input& input) override;

};