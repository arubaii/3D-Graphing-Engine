#include "Scene.h"
#include "scene_core/Entity.h"
#include "utils/Log.h"
#include "utils/Primitives.h"
#include "io/MouseCodes.h"
#include "surface/SurfaceEvaluator.h"

#include <cmath>
#include <algorithm>
#include <future>




Scene::Scene(Window& window, Input& input)
	: m_Window(window),
	  m_Viewport(m_Window.GetViewport()),
	  m_CameraProps{
		  45.0f,
		  window.GetAspectRatio(),
		  0.1f,
		  100000.0f
	  }
{
	m_CameraControllers.push_back(
		CreateScope<OrbitCameraController>(DefaultRadius, DefaultPitch, DefaultYaw)
	);
	m_ActiveController = 0;

	{
		Entity camera = CreateEntity(UUID(), "Main Camera");
		auto& cc = camera.AddComponent<CameraComponent>(
			m_CameraProps.Fov,
			m_CameraProps.AspectRatio,
			m_CameraProps.NearPlane,
			m_CameraProps.FarPlane
		);
		cc.Primary = true;
		//
		// cc.Camera.SetPosition(DefaultCameraPosition);
		// cc.Camera.SetRotation(DefaultPitch, DefaultYaw);
		// cc.Camera.RecalculateView();

		m_CameraControllers[m_ActiveController]->SetCamera(cc.Camera);
		m_CameraControllers[m_ActiveController]->OnSelect(glm::vec3(0.0f));
		m_CameraControllers[m_ActiveController]->Update(0.0f, input);

		Entity light = CreateEntity(UUID(), "Light");

		auto& lightComponent = light.AddComponent<MeshComponent>();
		lightComponent.MeshData = CreateRef<Mesh>();
		lightComponent.MeshData->Vertices = PRIMITIVES::LightVerts;
		lightComponent.MeshData->Indices  = PRIMITIVES::LightIdx;

		light.GetComponent<TransformComponent>().Scale = {5.0f, 5.0f, 5.0f};
		light.GetComponent<TransformComponent>().Translation = {5.0f, -2.5f, 10.0f};

		Entity yAxis = CreateEntity(UUID(), "Y-Axis");
		auto& yAxisComponent = yAxis.AddComponent<MeshComponent>();
		yAxisComponent.MeshData = CreateRef<Mesh>();
		yAxisComponent.MeshData->Vertices = PRIMITIVES::GetyAxisVertices(m_CameraProps.FarPlane);
		yAxisComponent.MeshData->Indices = PRIMITIVES::yAxisIndices;

		yAxis.GetComponent<TransformComponent>().Translation = {0.0f, -2.5f, 0.0f};

		Entity surface = CreateEntity(UUID(), "Surface");

		auto& mc = surface.AddComponent<MeshComponent>();
		auto& sc = surface.AddComponent<SurfaceComponent>();
		sc.Expression = CreateRef<MathParser::CompiledExpression>();
		sc.Mesh = CreateScope<SurfaceMesh>();
		sc.Dirty = true;

		mc.MeshData = CreateRef<Mesh>();

		surface.GetComponent<TransformComponent>().Translation = {0.0f, 0.0f, 0.0f};
	}

	{
		Entity box = CreateEntity(UUID(), "DomainBox");

		auto& mc = box.AddComponent<MeshComponent>();
		mc.MeshData = CreateRef<Mesh>();

		std::vector<Vertex> verts(8);
		verts[0].Position = {m_BoxMin.x, m_BoxMin.y, m_BoxMin.z};
		verts[1].Position = {m_BoxMax.x, m_BoxMin.y, m_BoxMin.z};
		verts[2].Position = {m_BoxMax.x, m_BoxMax.y, m_BoxMin.z};
		verts[3].Position = {m_BoxMin.x, m_BoxMax.y, m_BoxMin.z};
		verts[4].Position = {m_BoxMin.x, m_BoxMin.y, m_BoxMax.z};
		verts[5].Position = {m_BoxMax.x, m_BoxMin.y, m_BoxMax.z};
		verts[6].Position = {m_BoxMax.x, m_BoxMax.y, m_BoxMax.z};
		verts[7].Position = {m_BoxMin.x, m_BoxMax.y, m_BoxMax.z};

		std::vector<uint32_t> idx = {
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4, 1,5, 2,6, 3,7
		};

		mc.MeshData->Vertices = std::move(verts);
		mc.MeshData->Indices  = std::move(idx);
		mc.MeshData->Touch();
	}

	m_GridShader  	= Shader::Create("infinite_grid.vert", "infinite_grid.frag");
	m_LightShader 	= Shader::Create("light.vert", "light.frag");
	m_BaseShader  	= Shader::Create("base.vert", "base.frag");
	m_PhongShader 	= Shader::Create("phong.vert", "phong.frag");
	m_OutlineShader = Shader::Create("outline.vert", "outline.frag");

	m_CrosshairShader = Shader::Create("crosshair.vert", "crosshair.frag");
	m_CrosshairLayout.Push<float>(3);
	m_CrosshairVB = VertexBuffer(sizeof(PRIMITIVES::CrosshairVertices), PRIMITIVES::CrosshairVertices);
	m_CrosshairVA.AddBuffer(m_CrosshairVB, m_CrosshairLayout);
	glGenVertexArrays(1, &m_GridVAO);

	m_BoxContentScaleSurface = m_BoxContentScale;
}

void Scene::Render(Renderer& renderer)
{
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	Entity cam = GetPrimaryCameraEntity();
	if (!cam) return;
	auto& cc = cam.GetComponent<CameraComponent>();

	if (m_ShowGrid)
		DrawGrid(cc);

	glm::mat4 VP =
		cc.Camera.GetProjectionMatrix() *
		cc.Camera.GetViewMatrix();

	const glm::mat4 worldScale = glm::scale(glm::mat4(1.0f), glm::vec3(m_BoxContentScale));

	glm::vec3 lightPos{0.0f};
	glm::vec4 lightColor{1.0f};

	auto lightView = m_Registry.view<TransformComponent, TagComponent>();
	for (auto e : lightView)
	{
		auto& tag = lightView.get<TagComponent>(e);
		if (tag.Tag == "Light")
		{
			auto& tc = lightView.get<TransformComponent>(e);
			glm::mat4 lightModel = tc.GetTransform();
			lightPos = glm::vec3(lightModel[3]);
			break;
		}
	}

	auto view = m_Registry.view<TransformComponent, MeshComponent, TagComponent>();
	for (auto e : view)
	{
		auto& tag = m_Registry.get<TagComponent>(e);
		auto& tc  = m_Registry.get<TransformComponent>(e);
		auto& mc  = m_Registry.get<MeshComponent>(e);

		if (!mc.MeshData)
			continue;

		GPUMesh& gpu = MeshRendererCache::GetOrCreate(*mc.MeshData);

		glm::mat4 model = tc.GetTransform();
		glm::mat4 modelDraw = model;
		glm::mat4 MVP = VP * modelDraw;

		if (tag.Tag == "Light")
		{
			glm::mat4 lightModelAdj = glm::scale(modelDraw, glm::vec3(0.3f));

			m_LightShader->Bind();
			renderer.SetShader(m_LightShader);
			m_LightShader->SetMat4("u_MVP", VP * lightModelAdj);
			m_LightShader->SetVec4("u_LightColor", lightColor);
			renderer.Draw(gpu.VA, gpu.IB);
		}
		else if (tag.Tag == "Y-Axis" && m_ShowGrid)
		{
		}
		else if (tag.Tag == "DomainBox")
		{
			m_BaseShader->Bind();
			renderer.SetShader(m_BaseShader);
			m_BaseShader->SetMat4("u_MVP", MVP);
			m_BaseShader->SetMat4("u_Model", modelDraw);
			renderer.DrawLines(gpu.VA, gpu.IB);
		}
		else if (tag.Tag == "Surface")
		{

			m_PhongShader->Bind();
			renderer.SetShader(m_PhongShader);
			m_PhongShader->SetPhongUniforms(
				modelDraw,
				cc.Camera.GetProjectionMatrix(),
				lightPos,
				lightColor,
				cc.Camera
			);
			m_PhongShader->SetVec3("u_BoxMin", m_BoxMin);
			m_PhongShader->SetVec3("u_BoxMax", m_BoxMax);
			m_PhongShader->SetFloat("u_ContentScale", m_BoxContentScaleSurface);

			renderer.Draw(gpu.VA, gpu.IB);

			if (m_IsDragging)
			{
				glEnable(GL_POLYGON_OFFSET_LINE);
				glPolygonOffset(-1.0f, -1.0f);

				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_CULL_FACE);
				glLineWidth(2.0f);

				m_OutlineShader->Bind();
				renderer.SetShader(m_OutlineShader);
				m_OutlineShader->SetMat4("u_MVP", MVP);
				renderer.Draw(gpu.VA, gpu.IB);

				glLineWidth(1.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glDisable(GL_POLYGON_OFFSET_LINE);
			}
		}
		else
		{
			m_PhongShader->Bind();
			renderer.SetShader(m_PhongShader);
			m_PhongShader->SetPhongUniforms(
				modelDraw,
				cc.Camera.GetProjectionMatrix(),
				lightPos,
				lightColor,
				cc.Camera
			);
			renderer.Draw(gpu.VA, gpu.IB);
		}
	}

	DrawScreenOverlays(cc, renderer);
}

void Scene::Update(float dt, Input& input)
{
	m_RemeshCooldownLeft = std::max(0.0f, m_RemeshCooldownLeft - dt);

	Entity camEntity = GetPrimaryCameraEntity();
	if (!camEntity)
		return;

	auto& cc = camEntity.GetComponent<CameraComponent>();

	auto& controller = *m_CameraControllers[0];
	controller.SetCamera(cc.Camera);
	controller.Update(dt, input);

	CROSSHAIR_COLOR = glm::vec3(1.0f);

	float scroll = (float)input.GetMouseScrollY();
	if (scroll != 0.0f && !input.IsInUI())
	{
		const float step = 0.009f;
		m_ZoomLog2 += scroll * step;
		m_ZoomLog2 = std::clamp(m_ZoomLog2, -24.0f, 24.0f);
		m_BoxContentScale = exp2f(m_ZoomLog2);
	}

	{
		const float target = m_BoxContentScale;
		const float a = 1.0f - std::exp(-m_SurfaceZoomSmoothingHz * dt);
		m_BoxContentScaleSurface = m_BoxContentScaleSurface + (target - m_BoxContentScaleSurface) * a;
	}

	SurfaceSamplingConfig desired = ComputeSamplingConfig(cc.Camera);

	bool needRebuild = false;

	if (!m_HasLastSampling)
	{
		needRebuild = true;
		m_HasLastSampling = true;
	}
	else
	{
		if (m_SurfaceType == SurfaceType::Implicit)
		{
			const int oldN = m_LastSampling.s3.nx;
			const int newN = desired.s3.nx;
			if (std::abs(newN - oldN) >= 4) needRebuild = true;

			const float oldK = m_LastSampling.s3.contentScale;
			const float newK = desired.s3.contentScale;
			const float dk = std::abs(std::log2(std::max(newK, 1e-6f) / std::max(oldK, 1e-6f)));
			if (dk >= 0.125f) needRebuild = true;
		}
		else
		{
			const int oldN = m_LastSampling.s2.resolution;
			const int newN = desired.s2.resolution;
			if (std::abs(newN - oldN) >= 8) needRebuild = true;

			const float oldK = m_LastSampling.s2.contentScale;
			const float newK = desired.s2.contentScale;
			const float dk = std::abs(std::log2(std::max(newK, 1e-6f) / std::max(oldK, 1e-6f)));
			if (dk >= 0.125f) needRebuild = true;
		}

		m_DOMAIN_RADIUS = std::clamp(m_DOMAIN_RADIUS, MIN_DOMAIN_RADIUS, MAX_DOMAIN_RADIUS);
		if (std::abs(m_DOMAIN_RADIUS - m_LastDomainRadiusUsed) > 1e-4f)
			needRebuild = true;
	}

	if (needRebuild)
		m_SurfacePendingRemesh = true;

	if (m_SurfacePendingRemesh && m_RemeshCooldownLeft <= 0.0f)
	{
		m_LastSampling = desired;
		m_LastDomainRadiusUsed = m_DOMAIN_RADIUS;

		auto surfaceView2 = m_Registry.view<SurfaceComponent>();
		for (auto e2 : surfaceView2)
			surfaceView2.get<SurfaceComponent>(e2).Dirty = true;

		m_RemeshCooldownLeft = m_RemeshCooldownSec;
		m_SurfacePendingRemesh = false;
	}

	auto surfaceView = m_Registry.view<SurfaceComponent, MeshComponent>();
	for (auto e : surfaceView)
	{
		auto& sc = surfaceView.get<SurfaceComponent>(e);
		auto& mc = surfaceView.get<MeshComponent>(e);
		if (!sc.Dirty)
			continue;

		if (!sc.Expression)
		{
			LOG_ERROR("[Surface] Expression is null");
			sc.Dirty = false;
			continue;
		}

		// Async remesh
		if (!m_RemeshBusy.load())
		{
			const uint64_t jobId = ++m_RemeshJobId;

			const SurfaceType surfType = m_SurfaceType;
			const SurfaceSamplingConfig sampling = desired;

			const std::string exprStr = sc.Expression->get_expression_string();

			m_RemeshBusy.store(true);

			m_RemeshFuture = std::async(std::launch::async, [this, jobId, surfType, sampling, exprStr]()
			{
				auto expr = CreateRef<MathParser::CompiledExpression>();
				expr->set_expression(exprStr);

				SurfaceEvaluator eval(surfType, expr);

				SurfaceMesh temp;
				temp.Build(sampling, eval);

				auto verts = temp.GetVertices();
				auto idx   = temp.GetIndices();

				{
					std::lock_guard<std::mutex> lock(m_RemeshMutex);
					m_PendingMesh.verts = std::move(verts);
					m_PendingMesh.idx   = std::move(idx);
					m_PendingMesh.jobId = jobId;
					m_PendingMesh.ready.store(true);
				}

				m_RemeshBusy.store(false);
			});

			sc.Dirty = false;
		}


	}

	// Apply finished Mesh on main thrread
	if (m_PendingMesh.ready.load())
	{
		PendingMeshResult result;
		{
			std::lock_guard<std::mutex> lock(m_RemeshMutex);
			if (!m_PendingMesh.ready.load())
				return;

			result.jobId = m_PendingMesh.jobId;
			result.verts = std::move(m_PendingMesh.verts);
			result.idx   = std::move(m_PendingMesh.idx);
			m_PendingMesh.ready.store(false);
		}

		if (result.jobId == m_RemeshJobId.load())
		{
			auto surfaceView3 = m_Registry.view<SurfaceComponent, MeshComponent>();
			for (auto e3 : surfaceView3)
			{
				auto& mc = surfaceView3.get<MeshComponent>(e3);
				if (!mc.MeshData)
					continue;

				mc.MeshData->Vertices = std::move(result.verts);
				mc.MeshData->Indices  = std::move(result.idx);

				mc.MeshData->Touch();
				MeshRendererCache::Invalidate(*mc.MeshData);
				break;
			}
		}
	}

}

void Scene::DrawScreenOverlays(const CameraComponent& cc, Renderer& renderer)
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glDepthMask(GL_TRUE);

	m_CrosshairShader->Bind();
	m_CrosshairShader->SetVec3("u_Color", CROSSHAIR_COLOR);
	m_CrosshairVA.Bind();
	glLineWidth(10.0f);
	glDrawArrays(GL_LINES, 0, 4);
}

void Scene::DrawGrid(const CameraComponent& cc) const
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	glBindVertexArray(m_GridVAO);

	m_GridShader->Bind();
	m_GridShader->SetFloat("GridHeight", 0.0f);
	m_GridShader->SetVec3("u_BoxMin", m_BoxMin);
	m_GridShader->SetVec3("u_BoxMax", m_BoxMax);
	m_GridShader->SetFloat("u_ContentScale", m_BoxContentScale);
	m_GridShader->SetMat4("Projection", cc.Camera.GetProjectionMatrix());
	m_GridShader->SetMat4("View", cc.Camera.GetViewMatrix());
	m_GridShader->SetVec3("CameraWorldPos", cc.Camera.GetPosition());
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}


void Scene::SetPrimaryCamera(Entity entity)
{
	auto view = m_Registry.view<CameraComponent>();

	for (auto e : view)
		view.get<CameraComponent>(e).Primary = false;

	entity.GetComponent<CameraComponent>().Primary = true;
}

Entity Scene::GetPrimaryCameraEntity()
{
	auto view = m_Registry.view<CameraComponent>();
	for (auto e : view)
	{
		auto& component = view.get<CameraComponent>(e);
		if (component.Primary)
			return Entity{e, this};
	}

	return {};
}

glm::vec3 Scene::GetMainCameraPos()
{
	Entity camEntity = GetPrimaryCameraEntity();
	auto& cc = camEntity.GetComponent<CameraComponent>();
	glm::vec3 pos = cc.Camera.GetPosition();
	return pos;
}

float Scene::GetMainCameraPitch()
{
	Entity camEntity = GetPrimaryCameraEntity();
	auto& cc = camEntity.GetComponent<CameraComponent>();
	float pitch = cc.Camera.GetPitch();
	return pitch;
}

float Scene::GetMainCameraYaw()
{
	Entity camEntity = GetPrimaryCameraEntity();
	auto& cc = camEntity.GetComponent<CameraComponent>();
	float yaw = cc.Camera.GetYaw() - 180.0f;

	yaw = fmodf(yaw, 360.0f);
	if (yaw <= -180.0f) yaw += 360.0f;
	if (yaw >   180.0f) yaw -= 360.0f;

	return yaw;
}

SurfaceSamplingConfig Scene::ComputeSamplingConfigStatic()
{
	SurfaceSamplingConfig cfg;

	if (m_SurfaceType == SurfaceType::Implicit)
	{
		float r = m_DOMAIN_RADIUS;
		r = std::clamp(r, 0.5f, 200.0f);

		cfg.s3.min = {-r, -r, -r};
		cfg.s3.max = { r,  r,  r};
		cfg.s3.nx = cfg.s3.ny = cfg.s3.nz = 32;
		cfg.s3.iso = 0.0f;
	}
	else
	{
		cfg.s2 = ComputeSamplingFromCamera(GetPrimaryCameraEntity().GetComponent<CameraComponent>().Camera);
	}

	return cfg;
}

SurfaceSamplingConfig Scene::ComputeSamplingConfig(const PerspectiveCamera& cam)
{
	SurfaceSamplingConfig cfg{};

	switch (m_SurfaceType)
	{
		case SurfaceType::Implicit:
			cfg.s3 = ComputeSamplingFromCamera3D(cam);
			break;

		default:
			cfg.s2 = ComputeSamplingFromCamera(cam);
			break;
	}

	return cfg;
}

SurfaceSampling Scene::ComputeSamplingFromCamera(const PerspectiveCamera& cam)
{
	const glm::vec3 camPos = cam.GetPosition();
	const glm::vec3 pivot  = glm::vec3(0.0f);

	const float distance = glm::length(camPos - pivot);
	const float unitsPerPixel = cam.GetWorldUnitsPerPixel(distance, m_Viewport[1]);

	const float boxSpan = (m_BoxMax.x - m_BoxMin.x);
	const float domainSpan = 2.0f * m_DOMAIN_RADIUS;

	const float zoom = std::max(m_BoxContentScaleSurface, 1e-6f);

	const float k = (boxSpan > 0.0f) ? (domainSpan / boxSpan) / zoom : zoom;

	const float worldStep = unitsPerPixel * float(m_TargetPixelsPerSample);
	int N = (worldStep > 0.0f) ? int(boxSpan / worldStep) : m_MaxExplicitRes;
	N = std::clamp(N, m_MinExplicitRes, m_MaxExplicitRes);

	SurfaceSampling s{};
	s.xMin = m_BoxMin.x;
	s.yMin = m_BoxMin.z;
	s.xMax = m_BoxMax.x;
	s.yMax = m_BoxMax.z;
	s.resolution = N;
	s.contentScale = k;
	return s;
}

SurfaceSampling3D Scene::ComputeSamplingFromCamera3D(const PerspectiveCamera& cam)
{
	const glm::vec3 camPos = cam.GetPosition();
	const glm::vec3 center = {0.0f, 0.0f, 0.0f};

	const float distance = glm::length(camPos - center);
	const float unitsPerPixel = cam.GetWorldUnitsPerPixel(distance, m_Viewport[1]);

	const float boxSpan = (m_BoxMax.x - m_BoxMin.x);
	const float domainSpan = 2.0f * m_DOMAIN_RADIUS;

	const float zoom = std::max(m_BoxContentScaleSurface, 1e-6f);

	const float k = (boxSpan > 0.0f) ? (domainSpan / boxSpan) / zoom : zoom;

	const float worldStep = unitsPerPixel * float(m_TargetPixelsPerSample);
	int N = (worldStep > 0.0f) ? int(boxSpan / worldStep) : m_MaxImplicitRes;
	N = std::clamp(N, m_MinImplicitRes, m_MaxImplicitRes);

	SurfaceSampling3D s3{};
	s3.min = m_BoxMin;
	s3.max = m_BoxMax;
	s3.nx = s3.ny = s3.nz = N;
	s3.iso = 0.0f;
	s3.contentScale = k;
	return s3;
}

Entity Scene::InitEntity(const std::string& name)
{
	return CreateEntity(UUID(), name);
}

Entity Scene::CreateEntity(UUID uuid, const std::string& name)
{
	Entity entity = { m_Registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& tag = entity.AddComponent<TagComponent>();
	tag.Tag = name.empty() ? "Entity" : name;

	m_EntityMap[uuid] = entity;
	return entity;
}

void Scene::DeleteEntity(Entity entity)
{
	m_EntityMap.erase(entity.GetUUID());
	m_Registry.destroy(entity);
}

template<typename T>
void Scene::OnComponentAdded(Entity, T&) {}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
{
	if (component.Primary)
		SetPrimaryCamera(entity);
}