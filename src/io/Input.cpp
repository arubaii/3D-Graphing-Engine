#include "Input.h"
#include "../utils/Log.h"
#include <iostream>

Input* Input::s_Instance = nullptr;

void Input::Init(GLFWwindow* window)
{
	m_Window = window;
	s_Instance = this;
	// glfwSetWindowUserPointer(m_Window, this);
	glfwSetCursorPosCallback(m_Window, MouseCallback);
	glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	glfwSetScrollCallback(m_Window, ScrollCallback);
}

void Input::Update(double deltaTime)
{

	// Escape enables cursor
	if (IsKeyPressedOnce(GLFW_KEY_ESCAPE) && !s_Mouse.cursorEnabled)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		s_Mouse.cursorEnabled = true;
		s_Mouse.first = true;
	}
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

bool Input::IsMousePressed(int mouseButton) const
{
	return glfwGetMouseButton(m_Window, mouseButton);
}

bool Input::IsMousePressedOnce(int mouseButton)
{
	bool isDown  = IsMousePressed(mouseButton);
	bool wasDown = m_LastMouseButtonState[mouseButton];

	m_LastMouseButtonState[mouseButton] = isDown;

	return isDown && !wasDown;
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

	if (s_Instance->s_Mouse.cursorEnabled && action == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		s_Instance->s_Mouse.cursorEnabled = false;
		s_Instance->s_Mouse.first = true;
	}
}

// MouseScroll Input::s_Scroll{};
void Input::ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
	s_Scroll.X = xoffset;  // Horizontal scroll
	s_Scroll.Y = yoffset;
}

