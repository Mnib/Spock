#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "spock/window.hh"

namespace spock
{
    Window::Window(int width, int height, const char *title, bool resizable)
        : m_Window(nullptr)
        , m_Title(title)
        , m_Width(width)
        , m_Height(height)
        , m_Resizable(resizable) {
        Show();
    }

    Window::~Window() {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    static void FramebufferResized(GLFWwindow *window, int, int) {
        auto win = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        win->SetFramebufferResized(true);
    }

    void Window::Show() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        if (!m_Resizable)
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);
        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferResized);
    }

    bool Window::ShouldClose() const {
        if (glfwWindowShouldClose(m_Window)) {
            return true;
        }

        glfwPollEvents();

        return false;
    }
} // namespace spock
