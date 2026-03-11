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

	void InitImplicitFieldTex(int nx, int ny, int nz);

	void UpdateImplicitFieldVolume(const SurfaceEvaluator& eval, const SurfaceSampling3D& s3, int res);

	void OnImplicitExpressionChanged();

	void StartImplicitBake(const SurfaceEvaluator &eval, const SurfaceSampling3D &s3, int res);

	void PumpImplicitBake(int rowsPerFrame);

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
	Ref<Shader> m_BaseShader;
	Ref<Shader> m_CrosshairShader;
	Ref<Shader> m_LightShader;
	Ref<Shader> m_PhongShader;
	Ref<Shader> m_ImplicitRaymarchShader;

	VertexBufferLayout m_CrosshairLayout;
	VertexBuffer m_CrosshairVB;
	VertexArray m_CrosshairVA;

	glm::vec3 CROSSHAIR_COLOR = glm::vec3(1.0f);




	std::string m_LastImplicitExpr = "";
	bool m_ImplicitIsInfinite = false;

	GLuint m_ImplicitFieldTex = 0;
	int m_FieldNx = 0, m_FieldNy = 0, m_FieldNz = 0;

	// Infinite implicit (volume) update control
	bool  m_ImplicitVolDirty = true;          // needs upload
	bool  m_ImplicitVolPreview = true;        // while interacting
	float m_ImplicitVolUpdateTimer = 0.0f;    // throttling
	float m_ImplicitVolIdleTimer   = 0.0f;    // detect “stopped moving”

	float m_LastInfiniteK = 0.0f;
	glm::vec3 m_LastInfiniteMin = glm::vec3(0.0f);
	glm::vec3 m_LastInfiniteMax = glm::vec3(0.0f);

	glm::vec3 m_LastBakedMin = glm::vec3(-10.0f, -10.0f, -10.0f);
	glm::vec3 m_LastBakedMax = glm::vec3( 10.0f,  10.0f,  10.0f);
	float     m_LastBakedK   = 1.0f;



	GLuint m_FullscreenVAO = 0;

	struct ImplicitBakeState
	{
		bool active = false;
		int nx = 0, ny = 0, nz = 0;

		glm::vec3 mn{};
		glm::vec3 mx{};
		glm::vec3 span{};
		float iso = 0.0f;
		float k = 1.0f;

		int z = 0;          // current slice
		int y = 0;          // current row within slice

		std::vector<float> vox; // full volume buffer
		std::function<float(float,float,float)> F;

		bool uploadEachSlice = true;
	};

	ImplicitBakeState m_ImplicitBake;

public:
	static constexpr glm::vec3 DefaultCameraPosition{ 46.14f, 38.95f, 45.98f };
	static constexpr float     DefaultRadius =  30.0f;
	static constexpr float     DefaultPitch =  30.34;
	static constexpr float     DefaultYaw   = 228.20f;
	float m_ZoomLog2 = 0.0f;

	glm::vec3 BoxMin{-10.0f, -10.0f, -10.0f};
	glm::vec3 BoxMax{ 10.0f,  10.0f,  10.0f};
	float BoxContentScale = 1.0f;

	MathParser::CompiledExpression SurfaceExpression;

	SurfaceSamplingConfig LastSampling;
	bool HasLastSampling = false;
	bool ShowGrid = true;

	// LOD tuning
	int   TargetPixelsPerSample = 5;     // 2...6
	float MinDomainStep         = 0.01f;
	float MaxDomainStep         = 2.0f;

	int   MinExplicitRes        = 32;
	int   MaxExplicitRes        = 512;

	float MIN_DOMAIN_RADIUS = 0.5f;
	float MAX_DOMAIN_RADIUS = 2000.0f;

	int   LastExplicitRes       = -1;
	float LastDomainRadiusUsed  = -1.0f;

	float BoxContentScaleSurface = 1.0f;
	float SurfaceZoomSmoothingHz = 18.0f;   // 12...25 feels good
	float RemeshCooldownSec      = 0.04f;   // ~25 Hz max remesh
	float RemeshCooldownLeft     = 0.0f;
	bool  SurfacePendingRemesh   = false;

	int   ImplicitStaticRes = 2048;
	float MAX_IMPLICIT_RADIUS = 500.0f;

	bool ImplicitBuiltOnce = false;
	bool ImplicitNeedsBuild = true;

	bool ImplicitDomainInitialized = false;

	std::future<Mesh> ImplicitJob;
	std::atomic<bool> ImplicitJobRunning{false};
	std::atomic<bool> ImplicitJobReady{false};
	Mesh ImplicitJobResult;

	glm::mat4 ImplicitVP = glm::mat4(1.0f);
	bool ImplicitVPInitialized = false;

	bool ShowBox = false;

	glm::vec4 m_SurfaceColor;

	float m_OrbitRadius;

	float m_ContentZoomVelocity = 0.0f;
	float m_MouseScrollSensitivity = 0.12f;
	float m_KeyZoomSensitivity = 0.5f;
	float m_ZoomDamping = 8.0f;

	glm::vec3 m_GridBorderGreyscale;

	int OctreeDepth;

	// Tunables
	float m_InfinitePreviewHz = 20.0f;        // <= 10 updates/sec while scrolling
	float m_InfiniteIdleSettleSec = 0.20f;    // after 0.2s of no zoom -> HQ update
	int   m_InfinitePreviewRes;
	int   m_InfiniteFinalRes;                 // (128/160/192)

	float m_ClosedImplicitIdleTimer   = 0.0f;
	bool  m_ClosedImplicitVolDirty    = false;

	float m_ExplicitIdleTimer      = 0.0f;
	float m_ExplicitIdleSettleSec  = 0.15f;

	float GridTextureScale = 4.5f;
	float GridTextureWidth  = 1.0f;
	float GridTextureAlpha  = 0.20f;

	bool UsePastel;
	bool UseNormal;
	bool UserColor;


};

