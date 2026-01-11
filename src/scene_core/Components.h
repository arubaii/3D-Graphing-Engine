#pragma once
#include "GLcommon.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "PerspectiveCamera.h"
#include "renderer_core/IndexBuffer.h"
#include "renderer_core/Mesh.h"
#include "renderer_core/VertexArray.h"


struct TransformComponent
{
	glm::vec3 Translation {0.0f, 0.0f, 0.0f};
	glm::vec3 Rotation {0.0f, 0.0f, 0.0f};
	glm::vec3 Scale    {1.0f, 1.0f, 1.0f};

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent(const glm::vec3& translation)
		: Translation(translation) {}

	glm::mat4 GetTransform() const
	{
		glm::mat4 T = glm::translate(glm::mat4(1.0f), Translation);
		glm::mat4 R = glm::toMat4(glm::quat(Rotation));
		glm::mat4 S = glm::scale(glm::mat4(1.0f), Scale);

		return T * R * S; // TRS
	}
};

struct IDComponent
{
	UUID ID;
	IDComponent() = default;
	IDComponent(UUID uuid) : ID(uuid) {}
};

struct TagComponent
{
	std::string Tag;
};


struct MeshComponent
{
	std::shared_ptr<Mesh> MeshData;
};


struct CameraComponent
{
	PerspectiveCamera Camera;
	float Fov;
	float AspectRatio;
	float NearPlane;
	float FarPlane;

	bool Primary = false; // Denotes the primary camera in the scene

	CameraComponent(float Fov, float AspectRatio, float NearPlane, float FarPlane)
	: Camera(Fov, AspectRatio, NearPlane, FarPlane) {}
};
