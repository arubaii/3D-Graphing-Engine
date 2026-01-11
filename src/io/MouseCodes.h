#pragma once
#include <GLFW/glfw3.h>
#include <cstdint>

namespace Mouse
{
	enum Code : std::uint16_t
	{
		// Mouse buttons
		Button0 = GLFW_MOUSE_BUTTON_1,
		Button1 = GLFW_MOUSE_BUTTON_2,
		Button2 = GLFW_MOUSE_BUTTON_3,
		Button3 = GLFW_MOUSE_BUTTON_4,
		Button4 = GLFW_MOUSE_BUTTON_5,
		Button5 = GLFW_MOUSE_BUTTON_6,
		Button6 = GLFW_MOUSE_BUTTON_7,
		Button7 = GLFW_MOUSE_BUTTON_8,

		// Common aliases
		Left   = GLFW_MOUSE_BUTTON_LEFT,
		Right  = GLFW_MOUSE_BUTTON_RIGHT,
		Middle = GLFW_MOUSE_BUTTON_MIDDLE
	};
}