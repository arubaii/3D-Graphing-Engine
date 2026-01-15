#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include "KeyCodes.h"
#include "MouseCodes.h"

class Input
{
public:
	void Init(GLFWwindow* window);
	void Update(double deltaTime);
	static Input* s_Instance;

	bool IsKeyPressed		(int key)		  const;
	bool IsKeyPressedOnce	(int key);
	bool IsMousePressed	    (int mouseButton) const;
	bool IsMousePressedOnce (int mouseButton);

	bool CursorToggle(GLFWwindow* window);

	glm::vec2 GetMouseDelta();
	double GetMouseScrollY() { return s_Scroll.Y; }
	double GetMouseScrollX() { return s_Scroll.X; } // Useless for now
	glm::vec2 GetMousePos()  { return glm::vec2(s_Mouse.x, s_Mouse.y); }

	void EndFrame() { s_Scroll = {}; }
	bool IsCursorEnabled() const;
	bool IsKeyboardEnabled() const;


private:
	static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void ScrollCallback(GLFWwindow*, double xoffset, double yoffset);


	struct MouseState {
		double x = 0.0;
		double y = 0.0;
		double dx = 0.0;
		double dy = 0.0;
		bool first = true;
		bool cursorEnabled = false;

		// Buttons
		bool down[GLFW_MOUSE_BUTTON_LAST + 1]        = {};
		bool pressedOnce[GLFW_MOUSE_BUTTON_LAST + 1] = {};
		bool releasedOnce[GLFW_MOUSE_BUTTON_LAST + 1] = {};


		MouseState()
		: x(0.0), y(0.0), dx(0.0), dy(0.0), first(true), cursorEnabled(false) {}
	};

	struct MouseScroll
	{
		double X = 0.0;
		double Y = 0.0;
		MouseScroll() : X(0.0), Y(0.0) {}
	};


private:
	GLFWwindow* m_Window = nullptr;
	inline static MouseState  s_Mouse;
	inline static MouseScroll s_Scroll;
	std::unordered_map<int, bool> m_LastKeyState;
	std::unordered_map<int, bool> m_LastMouseButtonState;

	inline static bool s_KeyboardEnabled = true;


};

