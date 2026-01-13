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
}

Application::~Application() {}

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		UpdateDeltaTime();
		m_Window->PollEvents();


		m_Input.Update(m_DeltaTime);
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

		DebugPanel::Render(m_DebugData);
		m_ImGuiLayer->EndFrame();

		m_Input.EndFrame();
		m_Window->SwapBuffers();
	}
}


void Application::TestTriangle()
{
	static unsigned int vao = 0, vbo = 0;

	if (vao == 0)
	{

		float verts[] = {
			-0.5f, -0.5f, -1.0f,   1, 0, 0,
			 0.5f, -0.5f, -1.0f,   0, 1, 0,
			 0.0f,  0.5f, -1.0f,   0, 0, 1
		};


		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		// position attribute (location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);


		// color attribute (location = 1)
		glVertexAttribPointer(
			1,
			3,
			GL_FLOAT,
			GL_FALSE,
			6 * sizeof(float),
			(void*)(3 * sizeof(float))
		);
		glEnableVertexAttribArray(1);
	}

	glBindVertexArray(vao);
	m_Shader->Bind();

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 1.0f));


	// Get camera matrices (THIS is the key step)
	Entity cam = m_Scene->GetPrimaryCameraEntity();
	if (!cam)
		return;

	auto& cc = cam.GetComponent<CameraComponent>();

	glm::mat4 view = cc.Camera.GetViewMatrix();
	glm::mat4 proj = cc.Camera.GetProjectionMatrix();

	glm::mat4 mvp = proj * view * model;

	m_Shader->SetMat4("u_MVP", mvp);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

