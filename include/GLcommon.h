#pragma once

#ifdef __EMSCRIPTEN__
	// ImGui OpenGL backend: ES3 path
	#ifndef IMGUI_IMPL_OPENGL_ES3
	#define IMGUI_IMPL_OPENGL_ES3
	#endif

	#include <GLES3/gl3.h>
	#include <GLFW/glfw3.h>
#else
	// Tell ImGui’s OpenGL3 backend we use GLEW
	#ifndef IMGUI_IMPL_OPENGL_LOADER_GLEW
	#define IMGUI_IMPL_OPENGL_LOADER_GLEW
	#endif

	// Ensures we always include glew first
	#include <GL/glew.h>
	#include <GLFW/glfw3.h>
#endif
