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
	m_Window->SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	m_Shader     = CreateScope<Shader>("base.vert", "base.frag");
	m_Scene      = CreateScope<Scene>(*m_Window, m_Input);
	m_Renderer   = CreateScope<Renderer>();
	m_ImGuiLayer = CreateScope<ImGuiLayer>();

	m_ImGuiLayer->OnAttach(m_Window->GetGLFWwindow());
	m_DebugData.expression = CreateRef<MathParser::CompiledExpression>();
}

Application::~Application() {}

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		UpdateDeltaTime();
		m_Window->PollEvents();


		m_Input.Update();
		m_Scene->Update(m_DeltaTime, m_Input);
		glClearColor(m_DebugData.greyScale, m_DebugData.greyScale, m_DebugData.greyScale, 1.0f);
		m_Renderer->Clear();
		m_Scene->Render(*m_Renderer);


		m_ImGuiLayer->BeginFrame();

		m_DebugData.fps        = static_cast<int>(1.0f / m_DeltaTime);
		m_DebugData.frameTime  = m_DeltaTime * 1000.0f;
		m_DebugData.cameraPos  = m_Scene->GetMainCameraPos();
		m_DebugData.pitch      = m_Scene->GetMainCameraPitch();
		m_DebugData.yaw        = m_Scene->GetMainCameraYaw();

		m_Scene->m_DOMAIN_RADIUS = m_DebugData.radius;
		m_Scene->m_SurfaceType   = m_DebugData.surfaceType;
		m_Scene->m_ShowGrid	     = m_DebugData.showGrid;
		m_Scene->m_ShowBox	     = m_DebugData.showBox;

		DebugPanel::Render(m_DebugData);


		if (m_DebugData.expressionDirty)
		{
			m_Scene->SetSurfaceExpression(m_DebugData.expression);
			m_DebugData.expressionDirty = false;
		}


		m_ImGuiLayer->EndFrame();

		m_Input.EndFrame();
		m_Window->SwapBuffers();
	}
}



