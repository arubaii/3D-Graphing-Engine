#include <imgui.h>
#include <iostream>
#include "Input.h"
#include "utils/Log.h"

Input* Input::s_Instance = nullptr;

void Input::Init(GLFWwindow* window)
{
	m_Window = window;
	s_Instance = this;
	glfwSetCursorPosCallback(m_Window, MouseCallback);
	glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	glfwSetScrollCallback(m_Window, ScrollCallback);
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


	//
	// // Escape enables cursor
	// if (IsKeyPressedOnce(GLFW_KEY_ESCAPE))
	// {
	// 	if (!s_Mouse.cursorEnabled)
	// 		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	// 	else
	// 		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// 	s_Mouse.cursorEnabled = !s_Mouse.cursorEnabled;
	// 	s_Mouse.first = true;
	// }

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
	if (IsMousePressedOnce(Mouse::Left))
	{
		if (io.WantCaptureMouse)
		{
			m_IsInUI = true;
			s_KeyboardEnabled = false;
		}
		else
		{
			m_IsInUI = false;
			s_KeyboardEnabled = true;
		}
	}
	// ====================================================

	// When not in UI → keyboard always enabled
	if (!m_IsInUI)
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
	if (!s_Instance) return;

	auto& mouse = s_Instance->s_Mouse;

	if (mouse.first) {
		mouse.x = xpos;
		mouse.y = ypos;
		mouse.first = false;
		return;
	}

	mouse.dx = xpos - mouse.x;
	mouse.dy = mouse.y - ypos;
	mouse.x  = xpos;
	mouse.y  = ypos;

}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;

	if (!s_Instance) return;

	auto& mouse = s_Instance->s_Mouse;

}

void Input::ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
	s_Scroll.X = xoffset;  // Horizontal scroll
	s_Scroll.Y = yoffset;
}

