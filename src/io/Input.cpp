#include <imgui.h>
#include <iostream>
#include "Input.h"
#include "utils/Log.h"

#include "imgui_impl_glfw.h"

Input* Input::s_Instance = nullptr;

void Input::Init(GLFWwindow* window)
{
	m_Window = window;
	s_Instance = this;
	glfwSetCursorPosCallback(m_Window, MouseCallback);
	glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	glfwSetScrollCallback(m_Window, ScrollCallback);
	glfwSetKeyCallback(m_Window, KeyCallback);
	glfwSetCharCallback(m_Window, CharCallback);
}

void Input::Update()
{

	if (!s_Mouse.cursorEnabled)
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	if (IsKeyPressedOnce(Key::Escape))
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		s_Mouse.cursorEnabled = !s_Mouse.cursorEnabled;
		s_Mouse.first = true;
	}

	// Update mouse state
	for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b)
	{
		bool isDown = glfwGetMouseButton(m_Window, b) == GLFW_PRESS;

		s_Mouse.pressedOnce[b]  =  isDown && !s_Mouse.down[b];
		s_Mouse.releasedOnce[b] = !isDown &&  s_Mouse.down[b];
		s_Mouse.down[b] = isDown;
	}

	// UI activity state
	ImGuiIO& io = ImGui::GetIO();

	m_IsInUI = (io.WantCaptureMouse || io.WantCaptureKeyboard);

	if (m_IsInUI)
		s_KeyboardEnabled = false;
	else
		s_KeyboardEnabled = true;
}

bool Input::IsInUI() const
{
	return m_IsInUI;
}

bool Input::IsKeyPressed(int key) const
{
	return glfwGetKey(m_Window, key) == GLFW_PRESS;
}

bool Input::IsKeyPressedOnce(int key)
{

	bool isDown  = IsKeyPressed(key);
	bool wasDown = m_LastKeyState[key];

	m_LastKeyState[key] = isDown;

	return isDown && !wasDown;
}

bool Input::IsKeyboardEnabled() const
{
	return s_KeyboardEnabled;
}

bool Input::IsMousePressed(int button) const
{
	return s_Mouse.down[button];
}

bool Input::IsMousePressedOnce(int button)
{
	return s_Mouse.pressedOnce[button];
}

glm::vec2 Input::GetMouseDelta()
{
	glm::vec2 delta{
		static_cast<float>(s_Mouse.dx),
		static_cast<float>(s_Mouse.dy)
	};

	s_Mouse.dx = 0.0;
	s_Mouse.dy = 0.0;

	return delta;
}

bool Input::IsCursorEnabled() const
{
	return s_Mouse.cursorEnabled;
}

void Input::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

	if (!s_Instance) return;

	auto& mouse = s_Instance->s_Mouse;

	if (mouse.first) {
		mouse.x = xpos;
		mouse.y = ypos;
		mouse.first = false;
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	bool blockMouse = (io.WantCaptureMouse || s_Instance->m_IsInUI);

	if (blockMouse)
	{
		mouse.dx = 0.0;
		mouse.dy = 0.0;
		mouse.x  = xpos;
		mouse.y  = ypos;
		return;
	}

	mouse.dx = xpos - mouse.x;
	mouse.dy = mouse.y - ypos;
	mouse.x  = xpos;
	mouse.y  = ypos;

}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

	if (!s_Instance) return;

	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (action == GLFW_PRESS)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
		{
			s_Instance->m_IsInUI = true;
			s_Instance->s_Mouse.first = true;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
			s_Instance->m_IsInUI = false;
	}
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
		{
			s_Scroll.X = 0.0;
			s_Scroll.Y = 0.0;
			return;
		}
	}

	s_Scroll.X = xoffset;  // Horizontal scroll
	s_Scroll.Y = yoffset;
}

void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void Input::CharCallback(GLFWwindow* window, unsigned int c)
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui_ImplGlfw_CharCallback(window, c);
}