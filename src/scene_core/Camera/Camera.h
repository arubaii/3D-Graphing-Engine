#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "math/Ray.h"
#include "io/Input.h"

/*
	Camera
	|--- Perspective Camera <--- FreeCamera, OrbitCamera
			- Handles view and projection calculation

	Camera Controller
	| --- FreeCameraController, OrbitCameraController
			- Handles user I/O

*/
class Camera
{
public:
	virtual ~Camera() = default;

	// ================ Virtual interface =====================
	virtual const glm::mat4& GetViewMatrix() 		const = 0;
	virtual const glm::mat4& GetProjectionMatrix() 	const = 0;
	virtual const glm::vec3& GetPosition() 			const = 0;
	virtual 	  glm::vec3  GetForwardVector() 	const = 0;
	virtual 	  glm::vec3  GetRightVector() 		const = 0;
	virtual 	  glm::vec3  GetUpVector() 			const = 0;


	// Provided externally
	virtual void SetViewportSize(float width, float height)
	{
		m_ViewportWidth  = width;
		m_ViewportHeight = height;
	}

	float GetViewportWidth()  const { return m_ViewportWidth; }
	float GetViewportHeight() const { return m_ViewportHeight; }


	Ray GetRayFromScreen(const glm::vec2& cursorPos,
						 const glm::vec2& viewportSize) const
	{
		// 1. Screen -> NDC
		float x = (2.0f * cursorPos.x) / viewportSize.x - 1.0f;
		float y = 1.0f - (2.0f * cursorPos.y) / viewportSize.y;
		glm::vec2 ndc{x, y};

		// 2. Clip space
		glm::vec4 rayClip{ndc.x, ndc.y, -1.0f, 1.0f};

		// 3. View Space
		glm::vec4 rayView = glm::inverse(GetProjectionMatrix()) * rayClip;
		rayView = {rayView.x, rayView.y, -1.0f, 0.0f};

		glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(GetViewMatrix()) * rayView));

		return { GetPosition(), rayDir};
	}

protected:
	glm::mat4 m_View{1.0f};
	glm::mat4 m_Projection{1.0f};

	float m_ViewportWidth  = 1.0f;
	float m_ViewportHeight = 1.0f;
};