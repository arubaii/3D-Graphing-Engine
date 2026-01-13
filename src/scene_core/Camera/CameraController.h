#pragma once
#include "PerspectiveCamera.h"
#include "io/Input.h"

class CameraController
{
public:
	virtual ~CameraController() = default;
	virtual void Update(float dt, Input& input) = 0;
	virtual void OnActivate(Input&) {};
	virtual void OnSelect(const glm::vec3&) {}

	void SetCamera(PerspectiveCamera& camera) { m_Camera = &camera; }
protected:
	PerspectiveCamera* m_Camera = nullptr;
};