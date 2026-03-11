#pragma once
#include "CameraController.h"


class OrbitCameraController : public CameraController
{
private:
	glm::vec3 m_Pivot{0.0f};
	float m_Radius;
	float m_Pitch = 0.0f;
	float m_Yaw   = 0.0f;


	float m_MouseSensitivity       = 0.20f;
	float m_MouseScrollSensitivity = 0.001f;
	float m_KeySensitivity	       = 135.0f; // 135 deg per second
	float m_KeyZoomSensitivity     = 10.0f;
	float m_MinRadius              = 5.0f;
	float m_MaxRadius		       = 100000.0f;
	float m_ZoomVelocity		   = 0.0f;

	glm::vec2 m_AngularVelocity{};

public:
	explicit OrbitCameraController(float radius = 10.0f, float pitch = 0.0f, float yaw =0.0f)
	: m_Radius(radius), m_Yaw(yaw), m_Pitch(pitch) {}

	float GetYaw()   const { return m_Yaw; }
	float GetPitch() const { return m_Pitch; }

	void SetPivot(const glm::vec3& pivot) { m_Pivot = pivot; }
	void Update(float dt, Input& input) override;
	void OnActivate(Input& input) override;		// Reset mouse deltas
	void AddRadiusDelta(float d) override;

	void SetRadius (float radius) { m_Radius = radius; }

	void OnSelect(const glm::vec3& position) override { SetPivot(position); }



};