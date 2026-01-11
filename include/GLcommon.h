
#pragma once

// Tell ImGui’s OpenGL3 backend we use GLEW
#ifndef IMGUI_IMPL_OPENGL_LOADER_GLEW
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#endif

// Ensures we always include glew first
#include <GL/glew.h>
#include <GLFW/glfw3.h>
