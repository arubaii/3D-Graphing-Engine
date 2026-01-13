#pragma once
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "GLcommon.h"

class ImGuiLayer
{
public:
    // ImGuiLayer();
    // ~ImGuiLayer();

    void OnAttach(GLFWwindow* window);
    void OnDetach();

    void BeginFrame();
    void EndFrame();
};

