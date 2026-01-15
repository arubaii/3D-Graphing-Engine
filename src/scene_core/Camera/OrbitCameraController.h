#pragma once
#include "CameraController.h"


class OrbitCameraController : public CameraController
{
private:
	glm::vec3 m_Pivot{0.0f};
	float m_Radius;
	float m_Yaw   = 0.0f;
	float m_Pitch = 0.0f;
public:
	explicit OrbitCameraController(float radius = 10.0f)
	: m_Radius(radius) {}

	void SetPivot(const glm::vec3& pivot) { m_Pivot = pivot; }
	void Update(float dt, Input& input) override;
	void OnActivate(Input& input) override;		// Reset mouse deltas
	void OnSelect(const glm::vec3& position) override { SetPivot(position); }



};