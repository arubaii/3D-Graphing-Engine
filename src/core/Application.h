#pragma once
#include <string>
#include "Window.h"
#include "Renderer.h"
#include "Scene.h"
#include "io/Input.h"
#include "renderer_core/Shader.h"

struct ApplicationProperties
{
	std::string Name = "App";
	WindowProperties WindowProps;
};

class Application
{
public:
	Application(const ApplicationProperties& props);
	~Application();

	void onResize(int width, int height);

	Window& GetWindow() { return *m_Window; }
	void Run();
	void TestTriangle();
private:
	void UpdateDeltaTime()
	{
		float currentFrame = glfwGetTime();
		m_DeltaTime = currentFrame - m_LastFrame;
		m_LastFrame = currentFrame;
	}

private:
	ApplicationProperties m_AppProps;

	std::unique_ptr<Window>     m_Window;
	std::unique_ptr<Renderer>   m_Renderer;
	std::unique_ptr<Scene>      m_Scene;
	// std::unqiue_ptr<ImGuiLayer> m_ImGuiLayer;
	std::unique_ptr<Shader> m_Shader;
	Input	   m_Input;


	double m_LastFrame;
	double m_DeltaTime;

	int m_WindowWidth  = 0;
	int m_WindowHeight = 0;


};
