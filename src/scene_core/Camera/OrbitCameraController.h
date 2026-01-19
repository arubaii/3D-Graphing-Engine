#pragma once
#include "CameraController.h"


class OrbitCameraController : public CameraController
{
private:
	glm::vec3 m_Pivot{0.0f};
	float m_Radius;
	float m_Pitch = 0.0f;
	float m_Yaw   = 0.0f;
public:
	explicit OrbitCameraController(float radius = 10.0f, float pitch = 0.0f, float yaw =0.0f)
	: m_Radius(radius), m_Yaw(yaw), m_Pitch(pitch) {}

	float GetYaw()   const { return m_Yaw; }
	float GetPitch() const { return m_Pitch; }

	void SetPivot(const glm::vec3& pivot) { m_Pivot = pivot; }
	void Update(float dt, Input& input) override;
	void OnActivate(Input& input) override;		// Reset mouse deltas
	void OnSelect(const glm::vec3& position) override { SetPivot(position); }



};