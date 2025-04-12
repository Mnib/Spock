#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace spock
{
    class Window {
      public:
        Window(int width, int height, const char *title, bool resizable);
        ~Window();
        Window(const Window &) = delete;
        Window operator=(const Window &) = delete;

        void Show();
        bool ShouldClose() const;
        GLFWwindow *GetWindow() const {
            return m_Window;
        }

        bool WasFramebufferResized() const {
            return m_FramebufferResized;
        }

        void SetFramebufferResized(bool framebuffer_resized) {
            m_FramebufferResized = framebuffer_resized;
        }

      protected:
        GLFWwindow *m_Window;
        const char *m_Title;
        int m_Width;
        int m_Height;
        bool m_Resizable;
        bool m_FramebufferResized;
    };
} // namespace spock
