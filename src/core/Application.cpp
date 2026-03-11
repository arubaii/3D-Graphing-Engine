#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif
#include "utils/SmartPtrs.h"
#include "Application.h"
#include "utils/Log.h"
#include "scene_core/Entity.h"


Application::Application(const ApplicationProperties& props)
	: m_AppProps(props)
{
	m_Window = Window::Create
	(
		m_AppProps.WindowProps.Width,
		m_AppProps.WindowProps.Height,
		m_AppProps.WindowProps.Title,
		m_AppProps.WindowProps.MonitorSelected
	);

	m_Window->AttachInput(m_Input);
#ifndef __EMSCRIPTEN__
	m_Window->SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif

	m_Shader     = CreateScope<Shader>("base.vert", "base.frag");
	m_Scene      = CreateScope<Scene>(*m_Window, m_Input);
	m_Renderer   = CreateScope<Renderer>();
	m_ImGuiLayer = CreateScope<ImGuiLayer>();

	m_ImGuiLayer->OnAttach(m_Window->GetGLFWwindow());
	m_DebugData.expression = CreateRef<MathParser::CompiledExpression>();
}

Application::~Application() {}

void Application::Frame()
{
	try
	{
		UpdateDeltaTime();
		m_Window->PollEvents();

		m_Input.Update();
		m_Scene->Update(m_DeltaTime, m_Input);


		glClearColor(m_DebugData.greyScale, m_DebugData.greyScale, m_DebugData.greyScale, 1.0f);
		m_Renderer->Clear();

#ifdef __EMSCRIPTEN__
		{
			int fbW = 0, fbH = 0;
			emscripten_get_canvas_element_size("#canvas", &fbW, &fbH);
			if (fbW < 1) fbW = 1;
			if (fbH < 1) fbH = 1;
			glViewport(0, 0, fbW, fbH);
		}
#else
		{
			int fbW = m_Window->GetFBW();
			int fbH = m_Window->GetFBH();
			glViewport(0, 0, fbW, fbH);
		}
#endif

		glDisable(GL_SCISSOR_TEST);

		m_Scene->Render(*m_Renderer);


		m_ImGuiLayer->BeginFrame();

		m_DebugData.fps       = static_cast<int>(1.0f / m_DeltaTime);
		m_DebugData.frameTime = m_DeltaTime * 1000.0f;
		m_DebugData.cameraPos = m_Scene->GetMainCameraPos();
		m_DebugData.pitch     = m_Scene->GetMainCameraPitch();
		m_DebugData.yaw       = m_Scene->GetMainCameraYaw();

		m_Scene->m_DOMAIN_RADIUS 	   = m_DebugData.radius;
		m_Scene->m_SurfaceType   	   = m_DebugData.surfaceType;
		m_Scene->ShowGrid      	       = m_DebugData.showGrid;
		m_Scene->ShowBox       	       = m_DebugData.showBox;
		m_Scene->m_SurfaceColor  	   = m_DebugData.surfaceColor;
		m_Scene->m_OrbitRadius   	   = m_DebugData.radius;
		m_Scene->m_GridBorderGreyscale = glm::vec3(m_DebugData.greyScale);
		m_Scene->OctreeDepth           = m_DebugData.octreeDepth;
		m_Scene->m_InfiniteFinalRes    = m_DebugData.infiniteFinalRes;
		m_Scene->UsePastel = m_DebugData.usePastel;
		m_Scene->UseNormal = m_DebugData.useNormal;
		m_Scene->UserColor = m_DebugData.userColor;

		DebugPanel::Render(m_DebugData, m_Scene.get());

		if (m_DebugData.expressionDirty)
		{
			m_Scene->m_SurfaceType = m_DebugData.surfaceType;
			m_Scene->SetSurfaceExpression(m_DebugData.expression);
			m_DebugData.expressionDirty = false;
		}
		else
			m_Scene->m_SurfaceType = m_DebugData.surfaceType;

		m_ImGuiLayer->EndFrame();

		m_Input.EndFrame();
		m_Window->SwapBuffers();
	}
	catch (const std::exception& e)
	{
		LOG_ERROR(std::string("[Frame] ") + e.what());
		m_DebugData.expressionDirty = false;
		m_Input.EndFrame();
		m_Window->SwapBuffers();
	}
	catch (...)
	{
		LOG_ERROR("[Frame] Unknown exception");
		m_DebugData.expressionDirty = false;
		m_Input.EndFrame();
		m_Window->SwapBuffers();
	}
}

#ifdef __EMSCRIPTEN__
static void EmscriptenMainLoop(void* arg)
{
	static_cast<Application*>(arg)->Frame();
}

void Application::Run()
{
	emscripten_set_main_loop_arg(EmscriptenMainLoop, this, 0, true);


}
#else
void Application::Run()
{
	while (!m_Window->ShouldClose())
		Frame();
}
#endif



