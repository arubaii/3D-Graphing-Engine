#include "Window.h"
#include "io/Input.h"
#include "utils/Log.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#ifdef __EMSCRIPTEN__
static void SyncCanvasAndGlfwWindow(GLFWwindow* win, int& outFbW, int& outFbH)
{
    double cssW = 0.0, cssH = 0.0;
    emscripten_get_element_css_size("#canvas", &cssW, &cssH);

    if (cssW < 1.0) cssW = 1.0;
    if (cssH < 1.0) cssH = 1.0;

    double dpr = emscripten_get_device_pixel_ratio();
    if (dpr < 1.0) dpr = 1.0;

    int w = (int)(cssW + 0.5);
    int h = (int)(cssH + 0.5);

    int fbW = (int)(cssW * dpr + 0.5);
    int fbH = (int)(cssH * dpr + 0.5);


    glfwSetWindowSize(win, w, h);
    emscripten_set_canvas_element_size("#canvas", fbW, fbH);
    emscripten_set_element_css_size("#canvas", cssW, cssH);

    outFbW = fbW;
    outFbH = fbH;
}

static EM_BOOL OnEmscriptenResize(int, const EmscriptenUiEvent*, void* userData)
{
    Window* self = (Window*)userData;
    if (!self) return EM_FALSE;

    GLFWwindow* win = self->GetGLFWwindow();
    if (!win) return EM_FALSE;

    int fbW = 0, fbH = 0;
    SyncCanvasAndGlfwWindow(win, fbW, fbH);

    self->m_FramebufferWidth  = fbW;
    self->m_FramebufferHeight = fbH;

    glViewport(0, 0, fbW, fbH);
    return EM_TRUE;
}
#endif

Window::Window(const WindowProperties& props)
    : m_WindowProperties(props)
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);

#ifndef __EMSCRIPTEN__
    GLFWmonitor** monitors;
    int count;
    monitors = glfwGetMonitors(&count);

    if (!monitors || count == 0) {
        std::cerr << "ERROR: No monitors detected. Using default monitor 0.\n";
        m_WindowProperties.MonitorSelected = 0;
    }

    if (m_WindowProperties.MonitorSelected >= static_cast<unsigned int>(count)) {
        std::cerr << "WARNING: Invalid monitor index (" << m_WindowProperties.MonitorSelected
                  << "). Falling back to primary monitor (0)." << std::endl;
        m_WindowProperties.MonitorSelected = 0;
    }

    GLFWmonitor* monitor = monitors[m_WindowProperties.MonitorSelected];

    int xpos, ypos, workW, workH;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &workW, &workH);

    if (m_WindowProperties.Width == 0 || m_WindowProperties.Height == 0) {
        m_WindowProperties.Width  = workW;
        m_WindowProperties.Height = workH;
    }
#else
    if (m_WindowProperties.Width == 0 || m_WindowProperties.Height == 0)
    {
        m_WindowProperties.Width  = 1280;
        m_WindowProperties.Height = 720;
    }
#endif

    m_Window = glfwCreateWindow(m_WindowProperties.Width,
                               m_WindowProperties.Height,
                               m_WindowProperties.Title.c_str(),
                               nullptr,
                               nullptr);
    if (!m_Window)
        throw std::runtime_error("Failed to create GLFW window");

#ifndef __EMSCRIPTEN__
    int windowX = xpos + (workW - m_WindowProperties.Width) / 2;
    int windowY = ypos + (workH - m_WindowProperties.Height) / 2;
    glfwSetWindowPos(m_Window, windowX, windowY);
#endif

    glfwMakeContextCurrent(m_Window);

    glfwSwapInterval(0);

#ifndef __EMSCRIPTEN__
    glEnable(GL_MULTISAMPLE);
#endif

    glfwSetWindowUserPointer(m_Window, this);

#ifdef __EMSCRIPTEN__
    {
        int fbW = 0, fbH = 0;
        SyncCanvasAndGlfwWindow(m_Window, fbW, fbH);
        m_FramebufferWidth  = fbW;
        m_FramebufferHeight = fbH;
        glViewport(0, 0, fbW, fbH);
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnEmscriptenResize);
    }
#else
    {
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(m_Window, &fbw, &fbh);
        m_FramebufferWidth  = fbw;
        m_FramebufferHeight = fbh;
        glViewport(0, 0, fbw, fbh);
    }

    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* win, int w, int h) {
        glViewport(0, 0, w, h);
        if (auto* window = static_cast<Window*>(glfwGetWindowUserPointer(win))) {
            window->m_FramebufferWidth  = w;
            window->m_FramebufferHeight = h;
        }
    });

#ifndef __EMSCRIPTEN__
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        std::cout << "Failed to initialize GLEW" << std::endl;
#endif
#endif
}

Window::~Window()
{
    if (m_Window)
        glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::AttachInput(Input& input)
{
    input.Init(m_Window);
}

Scope<Window> Window::Create(unsigned int width,
                                       unsigned int height,
                                       const std::string& title,
                                       unsigned int MonitorSelected)
{
    WindowProperties spec;
    spec.Width = width;
    spec.Height = height;
    spec.Title = title;
    spec.MonitorSelected = MonitorSelected;

    return CreateScope<Window>(spec);
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }
void Window::SwapBuffers() const { glfwSwapBuffers(m_Window); }
void Window::PollEvents()  const { glfwPollEvents(); }

float Window::GetDPIScale() const
{
    int winW = 0, winH = 0;
    int fbW  = 0, fbH  = 0;

    glfwGetWindowSize(m_Window, &winW, &winH);
    glfwGetFramebufferSize(m_Window, &fbW, &fbH);

    if (winW == 0 || winH == 0) return 1.0f;

    float sx = float(fbW) / float(winW);
    float sy = float(fbH) / float(winH);
    return (sx + sy) * 0.5f;
}