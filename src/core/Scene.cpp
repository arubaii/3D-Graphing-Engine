#include "Scene.h"
#include "scene_core/Entity.h"
#include "utils/Log.h"
#include "utils/Primitives.h"
#include "io/MouseCodes.h"

// =========================== MAIN SCENE LOGIC =========================== //

Scene::Scene(Window& window, Input& input)
	 :  m_Window(window),
		m_CameraProps{
		45.0f,
		window.GetAspectRatio(),
		0.1f,
		1000.0f
		}

{

	m_CameraControllers.push_back(
	CreateScope<FreeCameraController>()
	);

	m_CameraControllers.push_back(
		CreateScope<OrbitCameraController>(10.0f)
	);
	m_ActiveController = FREE_CONTROLLER_INDEX;

	// Camera entity
	Entity camera = CreateEntity(UUID(), "Main Camera");
	auto& cc = camera.AddComponent<CameraComponent>(
		m_CameraProps.Fov,
		m_CameraProps.AspectRatio,
		m_CameraProps.NearPlane,
		m_CameraProps.FarPlane
	);
	cc.Primary = true;

	// Forces camera init config (sometimes slightly off before)
	cc.Camera.SetPosition(DefaultPosition);
	cc.Camera.SetRotation(DefaultPitch, DefaultYaw);
	cc.Camera.RecalculateView();
	m_CameraControllers[m_ActiveController]->OnActivate(input);



	Entity cube = CreateEntity(UUID(), "Cube");

	auto& mc = cube.AddComponent<MeshComponent>();
	mc.MeshData = CreateRef<Mesh>();
	mc.MeshData->Vertices = PRIMITIVES::CubeVertices;
	mc.MeshData->Indices  = PRIMITIVES::CubeIndices;


	// Load grid shader
	m_GridShader = Shader::Create("infinite_grid.vert", "infinite_grid.frag");
	m_BaseShader = Shader::Create("base.vert", "base.frag");

	cube.GetComponent<TransformComponent>().Translation = { 0.0f, 0.0f, -5.0f };



	m_CrosshairShader = Shader::Create("crosshair.vert", "crosshair.frag");
	m_CrosshairLayout.Push<float>(3);
	m_CrosshairVB = VertexBuffer(sizeof(PRIMITIVES::CrosshairVertices), PRIMITIVES::CrosshairVertices);
	m_CrosshairVA.AddBuffer(m_CrosshairVB, m_CrosshairLayout);
	glGenVertexArrays(1, &m_GridVAO);

}

void Scene::DrawGrid(const CameraComponent& cc) const
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	m_GridShader->Bind();
	m_GridShader->SetFloat("GridHeight", -2.5f);
	m_GridShader->SetMat4("Projection", cc.Camera.GetProjectionMatrix());
	m_GridShader->SetMat4("View", cc.Camera.GetViewMatrix());
	m_GridShader->SetVec3("CameraWorldPos", cc.Camera.GetPosition());


	glBindVertexArray(m_GridVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void Scene::Render(Renderer& renderer)
{
	glEnable(GL_DEPTH_TEST);
	Entity cam = GetPrimaryCameraEntity();
	if (!cam) return;
	auto& cc = cam.GetComponent<CameraComponent>();

	DrawGrid(cc);


	// ---- draw meshes ----
	glm::mat4 VP =
		cc.Camera.GetProjectionMatrix() *
		cc.Camera.GetViewMatrix();

	m_BaseShader->Bind();
	renderer.SetShader(m_BaseShader);
	auto view = m_Registry.view<TransformComponent, MeshComponent>();
	for (auto e : view)
	{
		auto& tc = m_Registry.get<TransformComponent>(e);
		auto& mc = m_Registry.get<MeshComponent>(e);

		GPUMesh& gpu = MeshRendererCache::GetOrCreate(*mc.MeshData);

		glm::mat4 MVP = VP * tc.GetTransform();
		m_BaseShader->SetMat4("u_MVP", MVP);

		renderer.Draw(gpu.VA, gpu.IB);

	}

	// CROSSHAIR
	glDisable(GL_DEPTH_TEST);


	m_CrosshairShader->Bind();
	m_CrosshairShader->SetVec3("color", glm::vec3(1.0f));
	m_CrosshairVA.Bind();
	glDrawArrays(GL_LINES, 0, 4);

}

void Scene::Update(float dt, Input& input)
{

	// TestCamera(dt, input);

	if (m_CameraControllers.empty())
		return;

	if (m_ActiveController >= m_CameraControllers.size())
		m_ActiveController = 0;

	Entity camEntity = GetPrimaryCameraEntity();
	if (!camEntity)
		return;

	auto& cc = camEntity.GetComponent<CameraComponent>();

	// ==================== Update Camera ======================
	auto& controller = *m_CameraControllers[m_ActiveController];
	controller.SetCamera(cc.Camera);
	controller.Update(dt, input);


	// ===================== Generate Ray =======================
	glm::vec2 mousePos = input.GetMousePos();
	glm::vec2 viewport = m_Window.GetViewPort();
	glm::vec2 center = viewport * 0.5f;
	Ray ray = cc.Camera.GetRayFromScreen(center, viewport);

	RayHit hit;
	bool hasHit = Raycast(ray, hit) && hit.entity != entt::null;

	// ======================================================================================= //
	// ================================== ORBIT SELECT LOGIC ================================= //
	// ======================================================================================= //

	if (hasHit)
	{
		Entity entity{ hit.entity, this };

		if (input.IsMousePressedOnce(Mouse::Middle))
		{
			if (m_ActiveController == ORBIT_CONTROLLER_INDEX)
				m_ActiveController = FREE_CONTROLLER_INDEX;   // exit orbit unconditionally
			else
			{
				m_ActiveController = ORBIT_CONTROLLER_INDEX;  // enter orbit
				m_CameraControllers[m_ActiveController]->OnSelect(entity.GetPosition());
			}
		}
	}

	// ======================================================================================= //
	// ======================================================================================= //

	// ============================================================= //
	// ===================== OBJECT DRAG LOGIC ===================== //
	// ============================================================= //

	// Capture Entity
	if (!m_IsDragging &&
		input.IsMousePressedOnce(Mouse::Left) &&
		m_ActiveController == FREE_CONTROLLER_INDEX &&
		hasHit)
	{
		m_DraggedEntity = hit.entity;
		m_IsDragging = true;

		Entity dragged{ m_DraggedEntity, this };
		m_DragOffset = dragged.GetPosition() - hit.position;
	}

	// Drag Entity
	// TODO: Refine Later
	//		 - Add cursor to move entity, so that the entire camera doesn't move
	//		 - Add a bounding area around the camera for more consistent dragging,
	//		   rather than a max distance
	constexpr float MAX_DRAG_DISTANCE = 25.0f;
	if (m_IsDragging &&
		input.IsMousePressed(Mouse::Left) &&
		m_DraggedEntity != entt::null)
	{
		glm::vec3 planeHit;
		if (IntersectPlane(ray, 0.0f, planeHit))
		{
			Entity dragged{ m_DraggedEntity, this };

			glm::vec3 desiredPos = planeHit + m_DragOffset;

			const glm::vec3 camPos = cc.Camera.GetPosition();
			glm::vec3 camToPos = desiredPos - camPos;

			float dist = glm::length(camToPos);
			if (dist > MAX_DRAG_DISTANCE)
				camToPos = glm::normalize(camToPos) * MAX_DRAG_DISTANCE;

			dragged.GetComponent<TransformComponent>().Translation =
				camPos + camToPos;
		}
	}

	// Release
	if (m_IsDragging && !input.IsMousePressed(Mouse::Left))
	{
		m_IsDragging = false;
		m_DraggedEntity = entt::null;
		m_DragOffset = glm::vec3(0.0f);
	}

	// ============================================================= //
	// ============================================================= //


	// // ============= Camera Type Swapping =============
	// if (input.IsMousePressedOnce(Mouse::Middle))
	// {
	// 	// Cycle through camera options
	// 	m_ActiveController =
	// 		(m_ActiveController + 1) % m_CameraControllers.size();
	// 	m_CameraControllers[m_ActiveController]->OnActivate(input);
	// }

}

void Scene::TestCamera(float dt, Input& input)
{
	static float printTimer = 0.0f;
	printTimer += dt;

	if (printTimer < 0.25)
		return;
	printTimer = 0.0f;

	Entity camEntity = GetPrimaryCameraEntity();
	if (!camEntity)
		return;

	const auto& cc = camEntity.GetComponent<CameraComponent>();

	const glm::vec3 pos = cc.Camera.GetPosition();
	const glm::vec3 fwd = cc.Camera.GetForwardVector();
	LOG(
		"Camera Pos: (",
		pos.x, ", ",
		pos.y, ", ",
		pos.z, ") | Forward: (",
		fwd.x, ", ",
		fwd.y, ", ",
		fwd.z, ")"
	);
}

bool Scene::Raycast(const Ray& ray, RayHit& outHit) const
{
	bool hitAnything = false;

	auto view = m_Registry.view<TransformComponent, MeshComponent>();
	for (auto e : view)
	{
		const auto& tc = view.get<TransformComponent>(e);
		const auto& mc = view.get<MeshComponent>(e);

		if (!mc.MeshData)
			continue;

		const glm::mat4 model = tc.GetTransform();

		const auto& vertices = mc.MeshData->Vertices;
		const auto& indices  = mc.MeshData->Indices;
		// LOG("Ray origin: ", ray.Origin);
		// LOG("Ray dir: ", ray.Direction);
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			glm::vec3 v0 = glm::vec3(model * glm::vec4(vertices[indices[i+0]].Position, 1.0f));
			glm::vec3 v1 = glm::vec3(model * glm::vec4(vertices[indices[i+1]].Position, 1.0f));
			glm::vec3 v2 = glm::vec3(model * glm::vec4(vertices[indices[i+2]].Position, 1.0f));

			float t;
			glm::vec3 normal;

			if (RayIntersectsTriangle(ray, v0, v1, v2, t, normal))
			{
				if (t < outHit.t)
				{
					outHit.Hit      = true;	     // Did we hit anything
					outHit.t        = t;
					outHit.position = ray.At(t); // Where did it hit
					outHit.normal   = normal;    // Orthonormal vector
					outHit.entity   = e;         // Who did we hit
					hitAnything     = true;		 // return value
				}
			}
		}
	}

	return hitAnything;
}


// ============================== ECS CONFIG ============================== //
void Scene::SetPrimaryCamera(Entity entity)
{
	auto view = m_Registry.view<CameraComponent>();

	// Clear all existing primary cameras
	for (auto e : view)
		view.get<CameraComponent>(e).Primary = false;

	// Set the new one
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

// Entity Scene::DuplicateEntity(Entity entity) { /* TODO */ }

void Scene::DeleteEntity(Entity entity)
{
	m_EntityMap.erase(entity.GetUUID());
	m_Registry.destroy(entity);
}

template<>
void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component) {}
template<>
void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component) {}
template<>
void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {}
template<>
void Scene::OnComponentAdded<MeshComponent>(Entity, MeshComponent&) {}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
{
	if (component.Primary)
		SetPrimaryCamera(entity);
}

