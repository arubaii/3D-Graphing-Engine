#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "io/Input.h"

/*
	Camera
	|--- Perspective Camera <--- FreeCamera, OrbitCamera
			- Handles view and projection calculation

	Camera Controller
	| --- FreeCameraController, OrbitCameraController
			- Handles user I/O

*/
class
Camera
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



protected:
	glm::mat4 m_View{1.0f};
	glm::mat4 m_Projection{1.0f};

	float m_ViewportWidth  = 1.0f;
	float m_ViewportHeight = 1.0f;
};