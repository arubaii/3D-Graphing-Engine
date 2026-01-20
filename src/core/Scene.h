#pragma once
#include <unordered_map>
#include <type_traits>
#include <cassert>
#include <entt/entt.hpp>
#include "utils/SmartPtrs.h"
#include "utils/UUID.h"
#include "scene_core/Components.h"
#include "Renderer.h"
#include "Window.h"
#include "io/Input.h"
#include "scene_core/Camera/OrbitCameraController.h"
#include "surface/SurfaceMesh.h"
#include "surface/SurfaceTypes.h"

#include <future>
#include <mutex>
#include <atomic>


class Entity;

struct CameraProps
{
	float Fov;
	float AspectRatio;
	float NearPlane;
	float FarPlane;
};

struct PendingMeshResult
{
	std::vector<Vertex>    verts;
	std::vector<uint32_t>  idx;
	std::atomic<bool>      ready{false};
	uint64_t               jobId = 0;
};


class Scene
{
public:

	void UpdateDomainBoxMesh(const glm::vec3& mn, const glm::vec3& mx);

	Scene(Window& window, Input& input);

	void DrawScreenOverlays(const CameraComponent &cc, Renderer& renderer);

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


	glm::vec3 GetMainCameraPos();
	float	  GetMainCameraPitch();
	float	  GetMainCameraYaw();

	SurfaceSamplingConfig ComputeSamplingConfigStatic();

	SurfaceSamplingConfig ComputeSamplingConfig(const PerspectiveCamera &cam);

	SurfaceSampling ComputeSamplingFromCamera(const PerspectiveCamera& cam);

	SurfaceSampling3D ComputeSamplingFromCamera3D(const PerspectiveCamera &cam);

	float m_DOMAIN_RADIUS = 25.0f;
	SurfaceType m_SurfaceType = SurfaceType::ExplicitXY;

	void SetSurfaceExpression(std::shared_ptr<MathParser::CompiledExpression> expr)
	{
		if (!expr)
			throw std::runtime_error("SetSurfaceExpression: expr is null");



		auto view = m_Registry.view<SurfaceComponent, MeshComponent>();
		for (auto e : view)
		{
			auto& sc = view.get<SurfaceComponent>(e);
			auto& tc = m_Registry.get<TransformComponent>(e);
			auto& mc = view.get<MeshComponent>(e);

			sc.Expression = expr;
			sc.Dirty = true;
			tc.Translation = {0.0f, 0.0f, 0.0f};

			if (mc.MeshData)
			{
				mc.MeshData->Vertices.clear();
				mc.MeshData->Indices.clear();
			}
		}
	}

	// float m_ImplicitRadius = 10.0f;
	// bool  m_HasImplicitRadius = false;

	std::future<void>   m_RemeshFuture;
	std::atomic<bool>   m_RemeshBusy{false};
	std::atomic<uint64_t> m_RemeshJobId{0};
	std::mutex          m_RemeshMutex;
	PendingMeshResult   m_PendingMesh;

private:
	Window&   m_Window;
	CameraProps m_CameraProps;

	glm::vec2 m_Viewport;

	// ============ Camera Configuration ============
	std::vector<Scope<CameraController>> m_CameraControllers;
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
	Ref<Shader> m_GridShader;
	Ref<Shader> m_BasisShader;
	Ref<Shader> m_BaseShader;
	Ref<Shader> m_CrosshairShader;
	Ref<Shader> m_LightShader;
	Ref<Shader> m_PhongShader;
	Ref<Shader> m_OutlineShader;

	VertexBufferLayout m_CrosshairLayout;
	VertexBuffer m_CrosshairVB;
	VertexArray m_CrosshairVA;

	glm::vec3 CROSSHAIR_COLOR = glm::vec3(1.0f);




	std::string m_LastImplicitExpr = "";


public:
	static constexpr glm::vec3 DefaultCameraPosition{ 46.14f, 38.95f, 45.98f };
	static constexpr float     DefaultRadius =  50.0f;
	static constexpr float     DefaultPitch =  30.34;
	static constexpr float     DefaultYaw   = 228.20f;
	float m_ZoomLog2 = 0.0f;

	glm::vec3 m_BoxMin{-10.0f, -10.0f, -10.0f};
	glm::vec3 m_BoxMax{ 10.0f,  10.0f,  10.0f};
	float m_BoxContentScale = 1.0f;

	MathParser::CompiledExpression m_SurfaceExpression;

	SurfaceSamplingConfig m_LastSampling;
	bool m_HasLastSampling = false;
	bool m_ShowGrid = true;

	// LOD tuning
	int   m_TargetPixelsPerSample = 4;     // 2...6
	float m_MinDomainStep         = 0.01f; // smallest spacing in "math/domain" units
	float m_MaxDomainStep         = 2.0f;  // largest spacing in "math/domain" units

	int   m_MinExplicitRes        = 16;
	int   m_MaxExplicitRes        = 256;

	float MIN_DOMAIN_RADIUS = 0.5f;
	float MAX_DOMAIN_RADIUS = 2000.0f;

	int   m_LastExplicitRes       = -1;
	float m_LastDomainRadiusUsed  = -1.0f;

	float m_BoxContentScaleSurface = 1.0f;
	float m_SurfaceZoomSmoothingHz = 18.0f;   // 12...25 feels good
	float m_RemeshCooldownSec      = 0.04f;   // ~25 Hz max remesh
	float m_RemeshCooldownLeft     = 0.0f;
	bool  m_SurfacePendingRemesh   = false;

	int   m_ImplicitStaticRes = 1024;
	float MAX_IMPLICIT_RADIUS = 500.0f;

	bool m_ImplicitBuiltOnce = false;
	bool m_ImplicitNeedsBuild = true;

	bool m_ImplicitDomainInitialized = false;

	std::future<Mesh> m_ImplicitJob;
	std::atomic<bool> m_ImplicitJobRunning{false};
	std::atomic<bool> m_ImplicitJobReady{false};
	Mesh m_ImplicitJobResult;

	glm::mat4 m_ImplicitVP = glm::mat4(1.0f);
	bool m_ImplicitVPInitialized = false;

	bool m_ShowBox = false;

};

