#pragma once
#include "GLcommon.h"
#include <glm/vec2.hpp>
#include <memory>
#include <iostream>


struct WindowProperties
{
	unsigned int Width  = 640;
	unsigned int Height = 640;
	std::string Title   = "OpenGL";
	unsigned int MonitorSelected = 0;

};


class Window
{
public:
	Window(const WindowProperties& props = WindowProperties());
	~Window();

	int	GetWidth()  const { return m_WindowProperties.Width; }
	int	GetHeight()	const { return m_WindowProperties.Height; }
	glm::vec2 GetViewPort() const
	{
		return glm::vec2(m_WindowProperties.Width, m_WindowProperties.Height);
	}
	float GetAspectRatio() const
	{
		if (m_FramebufferHeight == 0)
			return 1.0f;

		return (float)m_FramebufferWidth / (float)m_FramebufferHeight;
	}

	void SetInputMode         (int mode, int value) const
	{
		glfwSetInputMode(m_Window, mode, value);
	}

	void SetCursorPosCallback (GLFWcursorposfun callback) const
	{
		glfwSetCursorPosCallback(m_Window, callback);
	}
	void AttachInput(class Input& input);

	// void SetVSync(bool enabled = 0);


    static std::unique_ptr<Window> Create
	(
        unsigned int width,
        unsigned int height,
        const std::string& title,
        unsigned int monitor_selected
    );

	bool ShouldClose() const;
	void SwapBuffers() const;
	void PollEvents()  const;

private:
	GLFWwindow* m_Window = nullptr;
	WindowProperties m_WindowProperties;
	int m_FramebufferWidth;
	int m_FramebufferHeight;
};