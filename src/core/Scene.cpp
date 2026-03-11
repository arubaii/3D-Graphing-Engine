#include "Scene.h"
#include "scene_core/Entity.h"
#include "utils/Log.h"
#include "utils/Primitives.h"
#include "io/MouseCodes.h"
#include "surface/SurfaceEvaluator.h"

#include <cmath>
#include <algorithm>
#include <future>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

static float UnitsPerPixelAtDistance(float distance, float fovDeg, float viewportHeightPx)
{
    float fovRad = glm::radians(fovDeg);
    float halfHeightWorld = std::tan(fovRad * 0.5f) * distance;
    float heightWorld = halfHeightWorld * 2.0f;
    return heightWorld / std::max(viewportHeightPx, 1.0f);
}

static void BuildDomainBoxEdgeMesh(
    Mesh& mesh,
    const glm::vec3& bmin,
    const glm::vec3& bmax,
    const glm::vec3& cameraPos,
    float cameraFovDeg,
    float viewportHeightPx,
    float thicknessPx,
    const glm::vec3& color)
{


    glm::vec3 p[8];
    p[0] = {bmin.x, bmin.y, bmin.z};
    p[1] = {bmax.x, bmin.y, bmin.z};
    p[2] = {bmax.x, bmax.y, bmin.z};
    p[3] = {bmin.x, bmax.y, bmin.z};
    p[4] = {bmin.x, bmin.y, bmax.z};
    p[5] = {bmax.x, bmin.y, bmax.z};
    p[6] = {bmax.x, bmax.y, bmax.z};
    p[7] = {bmin.x, bmax.y, bmax.z};

    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    verts.reserve(12 * 8);
    idx.reserve(12 * 12);

    auto pushVertex = [&](const glm::vec3& pos)
    {
        Vertex v{};
        v.Position = pos;
        v.Normal   = glm::vec3(0.0f);
        v.TexCoord = glm::vec2(0.0f);
        v.Color    = color;
        verts.push_back(v);
    };

    auto addQuad = [&](const glm::vec3& A, const glm::vec3& B, const glm::vec3& off)
    {
        uint32_t base = (uint32_t)verts.size();
        pushVertex(A - off);
        pushVertex(A + off);
        pushVertex(B + off);
        pushVertex(B - off);

        idx.push_back(base + 0);
        idx.push_back(base + 1);
        idx.push_back(base + 2);

        idx.push_back(base + 0);
        idx.push_back(base + 2);
        idx.push_back(base + 3);
    };

    for (int e = 0; e < 12; ++e)
    {
        glm::vec3 A = p[edges[e][0]];
        glm::vec3 B = p[edges[e][1]];

        glm::vec3 edgeDir = B - A;
        float elen = glm::length(edgeDir);
        if (elen < 1e-6f)
            continue;
        edgeDir /= elen;

        glm::vec3 mid = (A + B) * 0.5f;

        // Convert desired thickness in pixels to world-units at this edge’s depth
        float dist = glm::length(cameraPos - mid);
        float unitsPerPx = UnitsPerPixelAtDistance(dist, cameraFovDeg, viewportHeightPx);
        float halfW = unitsPerPx * thicknessPx * 0.5f;

        glm::vec3 viewDir = cameraPos - mid;
        float vlen = glm::length(viewDir);
        if (vlen < 1e-6f)
            viewDir = glm::vec3(0.0f, 0.0f, 1.0f);
        else
            viewDir /= vlen;

        // Two orthonormal offsets perpendicular to the edge direction
        glm::vec3 offset1 = glm::cross(viewDir, edgeDir);
        float o1 = glm::length(offset1);
        if (o1 < 1e-6f)
        {
            glm::vec3 alt = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), edgeDir);
            float alen = glm::length(alt);
            if (alen < 1e-6f)
                alt = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), edgeDir);
            offset1 = alt;
            o1 = glm::length(offset1);
            if (o1 < 1e-6f)
                continue;
        }
        offset1 /= o1;

        glm::vec3 offset2 = glm::cross(edgeDir, offset1);
        float o2 = glm::length(offset2);
        if (o2 < 1e-6f)
            continue;
        offset2 /= o2;

        glm::vec3 off1 = offset1 * halfW;
        glm::vec3 off2 = offset2 * halfW;

        addQuad(A, B, off1);
        addQuad(A, B, off2);
    }

    mesh.Vertices = std::move(verts);
    mesh.Indices  = std::move(idx);
    mesh.Touch();
}

Scene::Scene(Window& window, Input& input)
    : m_Window(window),
      m_Viewport(m_Window.GetViewport()),
      m_CameraProps{
          90.0f,
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
        sc.Dirty = true; // triggers initial build when Update() runs

        mc.MeshData = CreateRef<Mesh>();

        surface.GetComponent<TransformComponent>().Translation = {0.0f, 0.0f, 0.0f};
    }

    {
        Entity box = CreateEntity(UUID(), "DomainBox");

        auto& mc = box.AddComponent<MeshComponent>();
        mc.MeshData = CreateRef<Mesh>();

        std::vector<Vertex> verts(8);
        verts[0].Position = {BoxMin.x, BoxMin.y, BoxMin.z};
        verts[1].Position = {BoxMax.x, BoxMin.y, BoxMin.z};
        verts[2].Position = {BoxMax.x, BoxMax.y, BoxMin.z};
        verts[3].Position = {BoxMin.x, BoxMax.y, BoxMin.z};
        verts[4].Position = {BoxMin.x, BoxMin.y, BoxMax.z};
        verts[5].Position = {BoxMax.x, BoxMin.y, BoxMax.z};
        verts[6].Position = {BoxMax.x, BoxMax.y, BoxMax.z};
        verts[7].Position = {BoxMin.x, BoxMax.y, BoxMax.z};

        std::vector<uint32_t> idx = {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4, 1,5, 2,6, 3,7
        };

        mc.MeshData->Vertices = std::move(verts);
        mc.MeshData->Indices  = std::move(idx);
        mc.MeshData->Touch();
    }

#ifdef __EMSCRIPTEN__
    m_GridShader              = Shader::Create("infinite_grid.vert", "infinite_grid.frag");
    m_LightShader             = Shader::Create("light.vert", "light.frag");
    m_BaseShader              = Shader::Create("base.vert", "base.frag");
    m_PhongShader             = Shader::Create("phong.vert", "phong.frag");
    m_ImplicitRaymarchShader  = Shader::Create("implicit_raymarch.vert", "implicit_raymarch.frag");
    m_CrosshairShader         = Shader::Create("crosshair.vert", "crosshair.frag");
#endif

    m_CrosshairShader = Shader::Create("crosshair.vert", "crosshair.frag");
    m_CrosshairLayout.Push<float>(3);
    m_CrosshairVB = VertexBuffer(sizeof(PRIMITIVES::CrosshairVertices), PRIMITIVES::CrosshairVertices);
    m_CrosshairVA.AddBuffer(m_CrosshairVB, m_CrosshairLayout);
    glGenVertexArrays(1, &m_GridVAO);
    glGenVertexArrays(1, &m_FullscreenVAO);

    BoxContentScaleSurface = BoxContentScale;
}

void Scene::Render(Renderer& renderer)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    Entity cam = GetPrimaryCameraEntity();
    if (!cam) return;

    auto& cc = cam.GetComponent<CameraComponent>();

    glm::vec3 camPos = cc.Camera.GetPosition();
    glm::vec3 camForward = cc.Camera.GetForwardVector();
    glm::vec3 lightPos = camPos + camForward * 2.0f + glm::vec3(0.0f, 1.5f, 0.0f);
    glm::vec4 lightColor{1.0f};

    // Keep a headlight-style light entity attached to camera
    auto lightView = m_Registry.view<TransformComponent, TagComponent>();
    for (auto e : lightView)
    {
        auto& tag = lightView.get<TagComponent>(e);
        if (tag.Tag == "Light")
        {
            auto& tc = lightView.get<TransformComponent>(e);
            tc.Translation = lightPos;
            break;
        }
    }

    if (ShowGrid)
        DrawGrid(cc);

    glm::mat4 VP =
        cc.Camera.GetProjectionMatrix() *
        cc.Camera.GetViewMatrix();

    // Explicit + finite implicit are drawn as actual meshes; this scales the mesh-space
    // so the domain zoom feels like “content zoom” rather than camera zoom.
    const glm::mat4 worldScale = glm::scale(glm::mat4(1.0f), glm::vec3(BoxContentScaleSurface));

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

        if (tag.Tag == "Surface" && m_SurfaceType == SurfaceType::Implicit && !m_ImplicitIsInfinite)
            modelDraw = worldScale * modelDraw;

        glm::mat4 MVP = VP * modelDraw;

        if (tag.Tag == "Light")
        {
            continue;
        }
        else if (tag.Tag == "Y-Axis" && ShowGrid)
        {
            continue;
        }
        else if (tag.Tag == "DomainBox")
        {
            m_BaseShader->Bind();
            renderer.SetShader(m_BaseShader);
            m_BaseShader->SetMat4("u_MVP", MVP);
            m_BaseShader->SetMat4("u_Model", modelDraw);

            if (ShowBox)
            {
                auto fb = m_Window.GetViewport();
                float viewportH = fb[1];

                // Rebuild edge-thickened box mesh each frame so thickness stays stable in pixels
                BuildDomainBoxEdgeMesh(
                    *mc.MeshData,
                    BoxMin,
                    BoxMax,
                    cc.Camera.GetPosition(),
                    m_CameraProps.Fov,
                    viewportH,
                    1.5f,
                    glm::vec3(0.60f, 0.60f, 0.60f)
                );

                MeshRendererCache::Invalidate(*mc.MeshData);

                GPUMesh& gpu2 = MeshRendererCache::GetOrCreate(*mc.MeshData);
                renderer.Draw(gpu2.VA, gpu2.IB);
            }
        }
        else if (tag.Tag == "Surface")
        {
            // Two rendering paths:
            // 1) Infinite implicit: raymarch a pre-baked 3D field texture
            // 2) Everything else: draw the CPU-generated mesh with Phong + optional grid overlay
            if (m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite)
            {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);

                m_ImplicitRaymarchShader->Bind();

                glm::mat4 invVP = glm::inverse(cc.Camera.GetProjectionMatrix() * cc.Camera.GetViewMatrix());

                m_ImplicitRaymarchShader->SetMat4("u_InvViewProj", invVP);
                m_ImplicitRaymarchShader->SetVec3("u_CamPos", cc.Camera.GetPosition());

                m_ImplicitRaymarchShader->SetVec3("u_BoxMin", BoxMin);
                m_ImplicitRaymarchShader->SetVec3("u_BoxMax", BoxMax);

                m_ImplicitRaymarchShader->SetFloat("u_Iso", 0.0f);
                m_ImplicitRaymarchShader->SetFloat("u_StepScale", 1.0f);

                m_ImplicitRaymarchShader->SetVec4("u_Color", m_SurfaceColor);
                m_ImplicitRaymarchShader->SetVec3("u_LightPos", lightPos);
                m_ImplicitRaymarchShader->SetVec4("u_LightColor", lightColor);
                m_ImplicitRaymarchShader->SetIVec3("u_FieldDim", glm::ivec3(m_FieldNx, m_FieldNy, m_FieldNz));
                m_ImplicitRaymarchShader->SetFloat("u_K", m_LastInfiniteK);

                m_ImplicitRaymarchShader->SetBool("usePastel", UsePastel);
                m_ImplicitRaymarchShader->SetBool("useNormal", UseNormal);
                m_ImplicitRaymarchShader->SetBool("userColor", UserColor);

                m_ImplicitRaymarchShader->SetFloat("u_GridScale", GridTextureScale);
                m_ImplicitRaymarchShader->SetFloat("u_GridWidth", GridTextureWidth);
                m_ImplicitRaymarchShader->SetVec3("u_GridColor", glm::vec3(0.0f));
                m_ImplicitRaymarchShader->SetFloat("u_GridAlpha", GridTextureAlpha);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_3D, m_ImplicitFieldTex);
                m_ImplicitRaymarchShader->SetInt("u_FieldTex", 0);

                glBindVertexArray(m_FullscreenVAO);
                glBindVertexArray(0);
                glDrawArrays(GL_TRIANGLES, 0, 3);

                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);

                continue;
            }

            m_PhongShader->Bind();
            renderer.SetShader(m_PhongShader);
            m_PhongShader->SetVec4("u_Color", m_SurfaceColor);
            m_PhongShader->SetPhongUniforms(
                modelDraw,
                cc.Camera.GetProjectionMatrix(),
                lightPos,
                lightColor,
                cc.Camera
            );

            m_PhongShader->SetBool("usePastel", UsePastel);
            m_PhongShader->SetBool("useNormal", UseNormal);
            m_PhongShader->SetBool("userColor", UserColor);

            m_PhongShader->SetMat4("u_MVP", MVP);
            m_PhongShader->SetMat4("u_Model", modelDraw);

            m_PhongShader->SetVec3("u_BoxMin", BoxMin);
            m_PhongShader->SetVec3("u_BoxMax", BoxMax);

            // Finite implicit uses BoxContentScaleSurface via modelDraw; explicit uses k to remap domain -> screen
            m_PhongShader->SetFloat("u_ContentScale", BoxContentScaleSurface);
            m_PhongShader->SetFloat("u_K", BoxContentScale);

            // Grid overlay parameters (world-space triplanar)
            m_PhongShader->SetFloat("u_GridScale", GridTextureScale);
            m_PhongShader->SetFloat("u_GridWidth", GridTextureWidth);
            m_PhongShader->SetVec3("u_GridColor", glm::vec3(0.0f));
            m_PhongShader->SetFloat("u_GridAlpha", GridTextureAlpha);

            renderer.Draw(gpu.VA, gpu.IB);
        }
    }

    // DrawScreenOverlays(cc, renderer);
}

void Scene::Update(float dt, Input& input)
{
    RemeshCooldownLeft = std::max(0.0f, RemeshCooldownLeft - dt);

    Entity camEntity = GetPrimaryCameraEntity();
    if (!camEntity)
        return;

    auto& cc = camEntity.GetComponent<CameraComponent>();

    auto& controller = *m_CameraControllers[0];
    controller.SetCamera(cc.Camera);
    controller.Update(dt, input);

    CROSSHAIR_COLOR = glm::vec3(1.0f);

    float scroll = (float)input.GetMouseScrollY();
    float inputDelta = 0.0f;

    static bool lastWasInfiniteImplicit = false;
    bool nowInfiniteImplicit = (m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite);

    // Reset zoom state when leaving infinite-implicit mode so explicit/finite behavior is consistent
    if (lastWasInfiniteImplicit && !nowInfiniteImplicit)
    {
        m_ContentZoomVelocity = 0.0f;
        m_ZoomLog2 = 0.0f;
        BoxContentScale = 1.0f;
        BoxContentScaleSurface = 1.0f;
    }

    lastWasInfiniteImplicit = nowInfiniteImplicit;

    // Force one rebuild when transitioning off infinite implicit
    if (lastWasInfiniteImplicit && !nowInfiniteImplicit)
    {
        SurfacePendingRemesh = true;
        RemeshCooldownLeft = 0.0f;

        auto surfaceView = m_Registry.view<SurfaceComponent>();
        for (auto e : surfaceView)
            surfaceView.get<SurfaceComponent>(e).Dirty = true;
    }

    if (!input.IsInUI())
    {
        const float boxSpan = std::max(BoxMax.x - BoxMin.x, BoxMax.z - BoxMin.z);
        const float minCell = boxSpan / 1024.0f;
        const float maxCell = boxSpan / 1.0f;
        const float gridBase = 2.0f;

        auto CellFromLog2 = [&](float zlog2) -> float
        {
            float scale = exp2f(zlog2);
            float cell = gridBase * std::max(scale, 1e-8f);
            return std::clamp(cell, minCell, maxCell);
        };

        if (scroll != 0.0f)
            inputDelta += scroll * m_MouseScrollSensitivity;

        if (input.IsKeyPressed(Key::Space))
            m_ContentZoomVelocity += m_KeyZoomSensitivity;
        else if (input.IsKeyPressed(Key::C))
            m_ContentZoomVelocity -= m_KeyZoomSensitivity;

        inputDelta += m_ContentZoomVelocity * dt;

        // Quantize zoom to cell sizes (prevents constant rebuilds for tiny wheel deltas)
        float curCell  = CellFromLog2(m_ZoomLog2);
        float nextCell = CellFromLog2(m_ZoomLog2 + inputDelta);

        bool blocked = (nextCell == curCell);

        if (m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite)
            blocked = false;

        if (!blocked)
        {
            m_ZoomLog2 += inputDelta;
        }

        float decay = std::exp(-m_ZoomDamping * dt);
        m_ContentZoomVelocity *= decay;
        if (std::abs(m_ContentZoomVelocity) < 0.001f)
            m_ContentZoomVelocity = 0.0f;

        m_ZoomLog2 = std::clamp(m_ZoomLog2, -24.0f, 24.0f);
        BoxContentScale = exp2f(m_ZoomLog2);
    }

    // Smooth the draw-scale separately from the “logical” zoom, so rendering doesn’t pop
    {
        const float target = BoxContentScale;
        const float a = 1.0f - std::exp(-SurfaceZoomSmoothingHz * dt);
        BoxContentScaleSurface = BoxContentScaleSurface + (target - BoxContentScaleSurface) * a;
    }

    SurfaceSamplingConfig desired{};

    // One-time implicit domain estimate per expression; marks whether to use infinite mode
    if (m_SurfaceType == SurfaceType::Implicit && !ImplicitDomainInitialized)
    {
        auto surfaceView = m_Registry.view<SurfaceComponent>();
        for (auto e : surfaceView)
        {
            auto& sc = surfaceView.get<SurfaceComponent>(e);
            if (sc.Expression)
            {
                sc.Expression->set_vars("x", 0.0);
                sc.Expression->set_vars("y", 0.0);
                sc.Expression->set_vars("z", 0.0);

                SurfaceEvaluator eval(SurfaceType::Implicit, sc.Expression);

                float r = eval.EstimateImplicitDomainRadius();
                r = std::clamp(r, MIN_DOMAIN_RADIUS, MAX_DOMAIN_RADIUS);

                m_DOMAIN_RADIUS = r;
                m_ImplicitIsInfinite = (r >= 0.90f * eval.GetMaxDomainRange());

                LastDomainRadiusUsed = r;
                ImplicitDomainInitialized = true;
                break;
            }
        }
    }

    // Sampling config drives both explicit mesh resolution and implicit volume bake bounds/k
    if (m_SurfaceType == SurfaceType::Implicit)
        desired = ComputeSamplingConfigStatic();
    else
        desired = ComputeSamplingConfig(cc.Camera);

    // Infinite implicit: rebuild the 3D field texture only when expr or bounds change (k alone is shader-side)
    if (m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite)
    {
        bool zoomingThisFrame = (std::abs(inputDelta) > 0.0f) || (std::abs(m_ContentZoomVelocity) > 0.0001f);

        std::string exprNow;
        {
            auto surfaceView = m_Registry.view<SurfaceComponent>();
            for (auto e : surfaceView)
            {
                auto& sc = surfaceView.get<SurfaceComponent>(e);
                if (sc.Expression)
                {
                    exprNow = sc.Expression->get_expression_string();
                    break;
                }
            }
        }

        const float kNow = desired.s3.contentScale;
        const bool exprChanged = (exprNow != m_LastImplicitExpr);

        bool boxChanged =
            (desired.s3.min != m_LastInfiniteMin) ||
            (desired.s3.max != m_LastInfiniteMax);

        if (exprChanged || boxChanged)
            m_ImplicitVolDirty = true;

        if (zoomingThisFrame)
        {
            m_ImplicitVolPreview = true;
            m_ImplicitVolIdleTimer = 0.0f;
        }
        else
        {
            m_ImplicitVolIdleTimer += dt;
        }

        m_LastInfiniteK    = kNow;
        m_LastInfiniteMin  = desired.s3.min;
        m_LastInfiniteMax  = desired.s3.max;
        m_LastImplicitExpr = exprNow;

        m_ImplicitVolUpdateTimer += dt;

        const float previewInterval = (m_InfinitePreviewHz > 0.0f) ? (1.0f / m_InfinitePreviewHz) : 0.0f;
        bool canPreviewUpdate = m_ImplicitVolPreview && (m_ImplicitVolUpdateTimer >= previewInterval);

        if (m_ImplicitVolPreview && (m_ImplicitVolIdleTimer >= m_InfiniteIdleSettleSec))
            m_ImplicitVolPreview = false;

        bool shouldFinalUpdate = (!m_ImplicitVolPreview) && (m_ImplicitVolIdleTimer >= m_InfiniteIdleSettleSec);

        if (m_ImplicitVolDirty && (canPreviewUpdate || shouldFinalUpdate))
        {
            auto surfaceView = m_Registry.view<SurfaceComponent>();
            for (auto e : surfaceView)
            {
                auto& sc = surfaceView.get<SurfaceComponent>(e);
                if (!sc.Expression) continue;

                SurfaceEvaluator eval(SurfaceType::Implicit, sc.Expression);

                const int res = m_ImplicitVolPreview ? m_InfinitePreviewRes : m_InfiniteFinalRes;
                StartImplicitBake(eval, desired.s3, res);
                break;
            }

            m_ImplicitVolDirty = false;
            m_ImplicitVolUpdateTimer = 0.0f;
        }
    }

    bool needRebuild = false;

    if (!HasLastSampling)
    {
        needRebuild = true;
        HasLastSampling = true;
    }
    else
    {
        if (m_SurfaceType == SurfaceType::Implicit)
        {
            if (m_SurfaceType == SurfaceType::Implicit)
            {
                const float oldK = LastSampling.s3.contentScale;
                const float newK = desired.s3.contentScale;

                const float dk = std::abs(std::log2(std::max(newK, 1e-6f) / std::max(oldK, 1e-6f)));
                if (dk >= 0.01f)
                    needRebuild = true;
            }
        }
        else
        {
            const int oldN = LastSampling.s2.resolution;
            const int newN = desired.s2.resolution;
            if (std::abs(newN - oldN) >= 8)
                needRebuild = true;

            const float oldK = LastSampling.s2.contentScale;
            const float newK = desired.s2.contentScale;
            const float dk =
                std::abs(std::log2(std::max(newK, 1e-6f) / std::max(oldK, 1e-6f)));
            if (dk >= 0.125f)
                needRebuild = true;
        }

        if (m_SurfaceType != SurfaceType::Implicit)
        {
            m_DOMAIN_RADIUS = std::clamp(m_DOMAIN_RADIUS, MIN_DOMAIN_RADIUS, MAX_DOMAIN_RADIUS);
            if (std::abs(m_DOMAIN_RADIUS - LastDomainRadiusUsed) > 1e-4f)
                needRebuild = true;
        }
    }

    if (needRebuild)
    {
        if (!(m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite))
            SurfacePendingRemesh = true;
    }

    if (SurfacePendingRemesh && RemeshCooldownLeft <= 0.0f)
    {
        LastSampling = desired;
        LastDomainRadiusUsed = m_DOMAIN_RADIUS;

        auto surfaceView2 = m_Registry.view<SurfaceComponent>();
        for (auto e2 : surfaceView2)
            surfaceView2.get<SurfaceComponent>(e2).Dirty = true;

        RemeshCooldownLeft = RemeshCooldownSec;
        SurfacePendingRemesh = false;
    }

    // CPU mesh generation for explicit + finite implicit; async job swaps into MeshData when ready
    if (!(m_SurfaceType == SurfaceType::Implicit && m_ImplicitIsInfinite))
    {
        auto surfaceView = m_Registry.view<SurfaceComponent, MeshComponent>();
        for (auto e : surfaceView)
        {
            auto& sc = surfaceView.get<SurfaceComponent>(e);
            if (!sc.Dirty)
                continue;

            if (!sc.Expression)
            {
                LOG_ERROR("[Surface] Expression is null");
                sc.Dirty = false;
                continue;
            }

            if (!m_RemeshBusy.load())
            {
                const uint64_t jobId = ++m_RemeshJobId;
                const SurfaceType surfType = m_SurfaceType;

                if (surfType == SurfaceType::Implicit)
                {
                    SurfaceEvaluator eval(surfType, sc.Expression);
                    float r = eval.EstimateImplicitDomainRadius();
                    r = std::clamp(r, MIN_DOMAIN_RADIUS, MAX_DOMAIN_RADIUS);
                    m_ImplicitIsInfinite = (r >= 0.90f * eval.GetMaxDomainRange());

                    m_DOMAIN_RADIUS = r;
                }

                const SurfaceSamplingConfig sampling = desired;
                const std::string exprStr = sc.Expression->get_expression_string();

                m_RemeshBusy.store(true);

#ifdef __EMSCRIPTEN__
                {
                    auto expr = CreateRef<MathParser::CompiledExpression>();
                    expr->set_expression(exprStr);
                    expr->set_vars("x", 0.0);
                    expr->set_vars("y", 0.0);
                    expr->set_vars("z", 0.0);

                    SurfaceEvaluator eval(surfType, expr);
                    if (m_ImplicitIsInfinite)
                    {
                        const int res = m_ImplicitVolPreview ? m_InfinitePreviewRes : m_InfiniteFinalRes;
                        StartImplicitBake(eval, desired.s3, res);
                    }
                    else
                    {
                        SurfaceMesh temp;
                        temp.Build(sampling, eval, OctreeDepth);

                        auto verts = temp.GetVertices();
                        auto idx   = temp.GetIndices();

                        {
                            std::lock_guard<std::mutex> lock(m_RemeshMutex);
                            m_PendingMesh.verts = std::move(verts);
                            m_PendingMesh.idx   = std::move(idx);
                            m_PendingMesh.jobId = jobId;
                            m_PendingMesh.ready.store(true);
                        }
                    }

                    m_RemeshBusy.store(false);
                }
#else
                m_RemeshFuture = std::async(std::launch::async, [this, jobId, surfType, sampling, exprStr]()
                {
                    auto expr = CreateRef<MathParser::CompiledExpression>();
                    expr->set_expression(exprStr);
                    expr->set_vars("x", 0.0);
                    expr->set_vars("y", 0.0);
                    expr->set_vars("z", 0.0);

                    SurfaceEvaluator eval(surfType, expr);

                    SurfaceMesh temp;
                    temp.Build(sampling, eval, OctreeDepth);

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
#endif

                sc.Dirty = false;
            }
        }

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

    PumpImplicitBake(32);
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
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_GridVAO);

    m_GridShader->Bind();
    m_GridShader->SetVec3("greyScale", m_GridBorderGreyscale);
    m_GridShader->SetFloat("GridHeight", 0.0f);
    m_GridShader->SetFloat("gGridSize", 400.0f);
    m_GridShader->SetVec3("u_BoxMin", BoxMin);
    m_GridShader->SetVec3("u_BoxMax", BoxMax);
    m_GridShader->SetFloat("u_ContentScale", BoxContentScale);
    m_GridShader->SetMat4("Projection", cc.Camera.GetProjectionMatrix());
    m_GridShader->SetMat4("View", cc.Camera.GetViewMatrix());
    m_GridShader->SetVec3("CameraWorldPos", cc.Camera.GetPosition());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
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
        cfg.s3.nx = cfg.s3.ny = cfg.s3.nz = ImplicitStaticRes;
        cfg.s3.iso = 0.0f;

        if (m_ImplicitIsInfinite)
        {
            cfg.s3.min = BoxMin;
            cfg.s3.max = BoxMax;

            const float boxSpan = (BoxMax.x - BoxMin.x);
            const float domainSpan = 2.0f * m_DOMAIN_RADIUS;
            const float zoom = std::max(BoxContentScaleSurface, 1e-6f);

            // Infinite implicit: shader-side zoom; contentScale is the mapping between world domain and box.
            cfg.s3.contentScale = (boxSpan > 0.0f) ? (domainSpan / boxSpan) * zoom : zoom;
        }
        else
        {
            float r = m_DOMAIN_RADIUS;
            cfg.s3.min = { -r, -r, -r };
            cfg.s3.max = {  r,  r,  r };
            cfg.s3.contentScale = 1.0f;
        }

        return cfg;
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
        {
            cfg.s3.nx = cfg.s3.ny = cfg.s3.nz = ImplicitStaticRes;
            cfg.s3.iso = 0.0f;

            if (m_ImplicitIsInfinite)
            {
                cfg.s3.min = BoxMin;
                cfg.s3.max = BoxMax;

                const float boxSpan   = (BoxMax.x - BoxMin.x);
                const float domainSpan = 2.0f * m_DOMAIN_RADIUS;
                const float zoom = std::max(BoxContentScaleSurface, 1e-6f);

                cfg.s3.contentScale = (boxSpan > 0.0f)
                    ? (domainSpan / boxSpan) / zoom
                    : (1.0f / zoom);
            }
            else
            {
                float r = m_DOMAIN_RADIUS;
                cfg.s3.min = { -r, -r, -r };
                cfg.s3.max = {  r,  r,  r };
                cfg.s3.contentScale = 1.0f;
            }

            return cfg;
        }

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

    const float boxSpan = (BoxMax.x - BoxMin.x);
    const float domainSpan = 2.0f * m_DOMAIN_RADIUS;

    const float zoom = std::max(BoxContentScaleSurface, 1e-6f);

    // k scales (x,y) domain evaluate, and counter-scale z so heights stay stable
    const float k = (boxSpan > 0.0f) ? (domainSpan / boxSpan) / zoom : zoom;

    const float worldStep = unitsPerPixel * float(TargetPixelsPerSample);
    int N = (worldStep > 0.0f) ? int(boxSpan / worldStep) : MaxExplicitRes;
    N = std::clamp(N, MinExplicitRes, MaxExplicitRes);

    SurfaceSampling s{};
    s.xMin = BoxMin.x;
    s.yMin = BoxMin.z;
    s.xMax = BoxMax.x;
    s.yMax = BoxMax.z;
    s.resolution = N;
    s.contentScale = k;
    return s;
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

void Scene::InitImplicitFieldTex(int nx, int ny, int nz)
{
    // 3D scalar field storage for infinite implicit raymarching (R32F).
    if (m_ImplicitFieldTex == 0)
        glGenTextures(1, &m_ImplicitFieldTex);

    m_FieldNx = nx; m_FieldNy = ny; m_FieldNz = nz;

    glBindTexture(GL_TEXTURE_3D, m_ImplicitFieldTex);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int maxDim = std::max(nx, std::max(ny, nz));
    int maxLevel = 0;
    while ((maxDim >> maxLevel) > 1) maxLevel++;

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nx, ny, nz, 0, GL_RED, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
}

void Scene::UpdateImplicitFieldVolume(const SurfaceEvaluator& eval, const SurfaceSampling3D& s3, int res)
{
    // Full rebuild of volume in one call
    const int nx = res, ny = res, nz = res;

    if (m_ImplicitFieldTex == 0 || nx != m_FieldNx || ny != m_FieldNy || nz != m_FieldNz)
        InitImplicitFieldTex(nx, ny, nz);

    std::vector<float> vox;
    vox.resize(size_t(nx) * size_t(ny) * size_t(nz));

    const glm::vec3 mn = s3.min;
    const glm::vec3 mx = s3.max;
    const glm::vec3 span = mx - mn;

    auto F = eval.GetCallableImplicit();

    const float iso = s3.iso;

    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
            {
                glm::vec3 uvw(
                    (x + 0.5f) / float(nx),
                    (y + 0.5f) / float(ny),
                    (z + 0.5f) / float(nz)
                );

                glm::vec3 p = mn + uvw * span;

                float v = F(p.x, p.y, p.z) - iso;
                if (!std::isfinite(v)) v = std::copysign(1e30f, v);

                vox[size_t(x) + size_t(nx) * (size_t(y) + size_t(ny) * size_t(z))] = v;
            }

    glBindTexture(GL_TEXTURE_3D, m_ImplicitFieldTex);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, GL_RED, GL_FLOAT, vox.data());
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Scene::OnImplicitExpressionChanged()
{
    // Reset all implicit-domain and volume-bake state so next Update() recomputes domain + schedules a bake.
    ImplicitDomainInitialized = false;
    m_ImplicitIsInfinite = false;

    m_ImplicitVolDirty = false;
    m_ImplicitVolPreview = false;
    m_ImplicitVolIdleTimer = 0.0f;
    m_ImplicitVolUpdateTimer = 0.0f;

    m_LastImplicitExpr.clear();
    m_LastInfiniteK = 0.0f;
    m_LastInfiniteMin = glm::vec3(0.0f);
    m_LastInfiniteMax = glm::vec3(0.0f);

    HasLastSampling = false;
    SurfacePendingRemesh = true;
    RemeshCooldownLeft = 0.0f;

    auto surfaceView = m_Registry.view<SurfaceComponent>();
    for (auto e : surfaceView)
        surfaceView.get<SurfaceComponent>(e).Dirty = true;
}

void Scene::StartImplicitBake(const SurfaceEvaluator& eval, const SurfaceSampling3D& s3, int res)
{
    // Initializes incremental rows-per-frame bake into m_ImplicitBake.vox, then PumpImplicitBake uploads.
    const int nx = res, ny = res, nz = res;

    if (m_ImplicitFieldTex == 0 || nx != m_FieldNx || ny != m_FieldNy || nz != m_FieldNz)
        InitImplicitFieldTex(nx, ny, nz);

    m_ImplicitBake.active = true;
    m_ImplicitBake.nx = nx; m_ImplicitBake.ny = ny; m_ImplicitBake.nz = nz;

    m_ImplicitBake.mn = s3.min;
    m_ImplicitBake.mx = s3.max;
    m_ImplicitBake.span = (s3.max - s3.min);

    m_ImplicitBake.iso = s3.iso;
    m_ImplicitBake.k = s3.contentScale;

    m_ImplicitBake.z = 0;
    m_ImplicitBake.y = 0;

    m_ImplicitBake.F = eval.GetCallableImplicit();

    m_ImplicitBake.vox.assign(size_t(nx) * size_t(ny) * size_t(nz), 0.0f);
}

void Scene::PumpImplicitBake(int rowsPerFrame)
{
    if (!m_ImplicitBake.active) return;

    const int nx = m_ImplicitBake.nx;
    const int ny = m_ImplicitBake.ny;
    const int nz = m_ImplicitBake.nz;

    const glm::vec3 mn = m_ImplicitBake.mn;
    const glm::vec3 span = m_ImplicitBake.span;

    auto& vox = m_ImplicitBake.vox;
    auto& F = m_ImplicitBake.F;

    const float iso = m_ImplicitBake.iso;

    int rowsDone = 0;

    // Fills Y-rows across X, advancing (y,z). Upload strategy is controlled by uploadEachSlice.
    while (m_ImplicitBake.z < nz && rowsDone < rowsPerFrame)
    {
        int z = m_ImplicitBake.z;
        int y = m_ImplicitBake.y;

        for (int x = 0; x < nx; ++x)
        {
            glm::vec3 uvw(
                (x + 0.5f) / float(nx),
                (y + 0.5f) / float(ny),
                (z + 0.5f) / float(nz)
            );

            glm::vec3 p = mn + uvw * span;

            float v = F(p.x, p.y, p.z) - iso;
            if (!std::isfinite(v)) v = std::copysign(1e30f, v);

            vox[size_t(x) + size_t(nx) * (size_t(y) + size_t(ny) * size_t(z))] = v;
        }

        m_ImplicitBake.y++;
        rowsDone++;

        if (m_ImplicitBake.y >= ny)
        {
            if (m_ImplicitBake.uploadEachSlice)
            {
                glBindTexture(GL_TEXTURE_3D, m_ImplicitFieldTex);

                const float* slicePtr = vox.data() + size_t(nx) * size_t(ny) * size_t(z);

                glTexSubImage3D(
                    GL_TEXTURE_3D,
                    0,
                    0, 0, z,
                    nx, ny, 1,
                    GL_RED,
                    GL_FLOAT,
                    slicePtr
                );

                glBindTexture(GL_TEXTURE_3D, 0);
            }

            m_ImplicitBake.y = 0;
            m_ImplicitBake.z++;
        }
    }

    if (m_ImplicitBake.z >= nz)
    {
        if (!m_ImplicitBake.uploadEachSlice)
        {
            glBindTexture(GL_TEXTURE_3D, m_ImplicitFieldTex);
            glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, GL_RED, GL_FLOAT, vox.data());
            glGenerateMipmap(GL_TEXTURE_3D);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glBindTexture(GL_TEXTURE_3D, 0);
        }

        m_ImplicitBake.active = false;
    }
}