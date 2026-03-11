#include "ui/ImGuiLayer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

void ImGuiLayer::OnAttach(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, false);

#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#if defined(EMSCRIPTEN_USE_EMBEDDED_GLFW3)
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
#else
    ImGui_ImplOpenGL3_Init("#version 410");
#endif
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame(); // sets io.DisplaySize + io.DisplayFramebufferScale automatically

#ifdef __EMSCRIPTEN__
    {
        double dpr = emscripten_get_device_pixel_ratio();
        if (dpr < 1.0) dpr = 1.0;

        double cssW = 0.0, cssH = 0.0;
        emscripten_get_element_css_size("#canvas", &cssW, &cssH);
        if (cssW < 1.0) cssW = 1.0;
        if (cssH < 1.0) cssH = 1.0;

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize             = ImVec2((float)cssW, (float)cssH);
        io.DisplayFramebufferScale = ImVec2((float)dpr,  (float)dpr);
    }
#endif

    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}