#include "Window.h"
#include "io/Input.h"

Window::Window(const WindowProperties& props)
    : m_WindowProperties(props)
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // REQUIRED on macOS
    // ===== Monitor and Window Size Query =====
    GLFWmonitor** monitors;
    int count;
    monitors = glfwGetMonitors(&count);

    if (!monitors || count == 0) {
        std::cerr << "ERROR: No monitors detected. Using default monitor 0.\n";
        m_WindowProperties.MonitorSelected = 0;
    }

    // Falls back to primary monitor rather than segfaulting
    if (m_WindowProperties.MonitorSelected >= static_cast<unsigned int>(count)) {
        std::cerr << "WARNING: Invalid monitor index (" << m_WindowProperties.MonitorSelected
                  << "). Falling back to primary monitor (0)." << std::endl;
        m_WindowProperties.MonitorSelected = 0;
    }

    GLFWmonitor* monitor = monitors[m_WindowProperties.MonitorSelected];

    int xpos, ypos, workW, workH;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &workW, &workH);

    // Auto fallback: use full work area if width/height not specified
    if (m_WindowProperties.Width == 0 || m_WindowProperties.Height == 0) {
        m_WindowProperties.Width  = workW;
        m_WindowProperties.Height = workH;
    }

    // ===== Window Creation =====
    m_Window = glfwCreateWindow(m_WindowProperties.Width,
                               m_WindowProperties.Height,
                               m_WindowProperties.Title.c_str(),
                               nullptr,
                               nullptr);
    if (!m_Window)
        throw std::runtime_error("Failed to create GLFW window");

    int windowX = xpos + (workW - m_WindowProperties.Width) / 2;
    int windowY = ypos + (workH - m_WindowProperties.Height) / 2;

    glfwSetWindowPos(m_Window, windowX, windowY); // Center of window


    glfwMakeContextCurrent(m_Window);
    int fbw, fbh;
    glfwGetFramebufferSize(m_Window, &fbw, &fbh);

    m_FramebufferWidth  = fbw;
    m_FramebufferHeight = fbh;
    glfwSwapInterval(0); // Glitchy, keep off for now

    // IMPORTANT: set initial viewport (Retina screens need framebuffer size)
    float xscale, yscale;
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);

    // compute framebuffer size for this monitor
    int fbW = static_cast<int>(workW * xscale);
    int fbH = static_cast<int>(workH * yscale);

    glfwGetFramebufferSize(m_Window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        std::cout << "Failed to initialize GLEW" << std::endl;

    // ===== Resize callback =====
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* win, int w, int h) {
        glViewport(0, 0, w, h);
        if (auto* window = static_cast<Window*>(glfwGetWindowUserPointer(win))) {
            window->m_FramebufferWidth  = w;
            window->m_FramebufferHeight = h;
        }
    });
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

std::unique_ptr<Window> Window::Create(unsigned int width,
                                       unsigned int height,
                                       const std::string& title,
                                       unsigned int MonitorSelected)
{
    WindowProperties spec;
    spec.Width = width;
    spec.Height = height;
    spec.Title = title;
    spec.MonitorSelected = MonitorSelected;

    return std::make_unique<Window>(spec);
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }
void Window::SwapBuffers() const { glfwSwapBuffers(m_Window); }
void Window::PollEvents()  const { glfwPollEvents(); }

