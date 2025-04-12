#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <chrono>
#include <fmt/base.h>
#include <imgui.h>
#include <memory>

#include "spock/application.hh"
#include "spock/layer.hh"
#include "spock/spock.hh"
#include "spock/window.hh"

namespace spock
{
    Application::Application(const SpockSettings &settings) {
        Spock::Initialize(settings);
    }

    void Application::Run() {
        int w, h;
        ImGui_ImplVulkanH_Window wd;
        auto &window = Spock::GetWindow();
        glfwGetFramebufferSize(window->GetWindow(), &w, &h);

        // While window is opened
        while (!window->ShouldClose()) {
            // Begin frame
            auto command_buffer = Spock::BeginFrame();

            // Compute delta time
            static auto last_update = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();

            float delta_time =
                std::chrono::duration<float, std::chrono::milliseconds::period>(now - last_update).count();
            last_update = now;

            // Run update
            for (std::shared_ptr<Layer> &layer : m_Layers)
                layer->OnUpdate(delta_time);

            // Render frames
            for (auto &layer : m_Layers) {
                layer->OnRender(command_buffer);
            }

            // Render UI
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            for (auto &layer : m_Layers) {
                layer->OnUIRender(command_buffer);
            }

            ImGui::Render();
            ImDrawData *main_draw_data = ImGui::GetDrawData();
            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(main_draw_data, command_buffer);

            Spock::EndFrame(command_buffer);
        }

        Spock::WaitForIdle();
    }

    void Application::PushLayer(const std::shared_ptr<Layer> layer) {
        m_Layers.emplace_back(layer)->OnAttach();
    }

    Application::~Application() {
        m_Layers.clear();

        Spock::Cleanup();
    }
} // namespace spock
