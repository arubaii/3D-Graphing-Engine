#pragma once
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <cassert>
#include <entt/entt.hpp>

#include "utils/UUID.h"
#include "scene_core/Components.h" // Move to .cpp later
#include "Renderer.h"
#include "Window.h"
#include "io/Input.h"
#include "scene_core/FreeCameraController.h"
#include "scene_core/OrbitCameraController.h"
#include "math/RayHit.h"
#include "math/Intersect.h"



class Entity; // Forward decl

struct CameraProps
{
	float Fov;
	float AspectRatio;
	float NearPlane;
	float FarPlane;
};

class Scene
{
public:
	Scene(Window& window, Input& input);

	void Update(float dt, Input& input);
	void Render(Renderer& renderer);

	// ============ Entity Configuration ============
	Entity InitEntity(const std::string& name = "Unnamed Entity");
	Entity CreateEntity(UUID uuid, const std::string& name = "Unnamed Entity");

	Entity GetEntityByName(std::string_view name);
	Entity GetEntityByUUID(UUID uuid);
	// Entity DuplicateEntity(Entity entity); // TODO
	void   DeleteEntity   (Entity entity);

	void SetPrimaryCamera(Entity entity);

	Entity GetPrimaryCameraEntity(); // Useful for multiple cameras

	template<class T>
	void OnComponentAdded(Entity entity, T &component);

	void DrawGrid(const CameraComponent& cc) const;
	void TestCamera(float dt, Input& input);

	bool Raycast(const Ray& ray, RayHit& outHit) const;

private:
	Window&   m_Window;
	CameraProps m_CameraProps;


	// ============ Camera Configuration ============
	std::vector<std::unique_ptr<CameraController>> m_CameraControllers;
	std::size_t m_ActiveController = 0;
	std::size_t FREE_CONTROLLER_INDEX = 0;
	std::size_t ORBIT_CONTROLLER_INDEX = 1;
	float MOVE_PLANE_Y = 0.0f;


	friend class Entity;
	entt::registry m_Registry;
	std::unordered_map<UUID, entt::entity> m_EntityMap;

	// Click and hold to move around
	entt::entity m_DraggedEntity = entt::null;
	bool         m_IsDragging = false;
	glm::vec3 m_DragOffset{0.0f};

	unsigned int m_GridVAO = 0;
	std::shared_ptr<Shader> m_GridShader;
	std::shared_ptr<Shader> m_BaseShader;
	std::shared_ptr<Shader> m_CrosshairShader;

	VertexBufferLayout m_CrosshairLayout;
	VertexBuffer m_CrosshairVB;
	VertexArray m_CrosshairVA;
public:
	// Camera, see perspective camera for defaults --> refactor later
	static constexpr glm::vec3 DefaultPosition{ 0.0f, 0.0f, 3.0f };
	static constexpr float     DefaultPitch = 0.0f;
	static constexpr float     DefaultYaw   = -90.0f;



};

