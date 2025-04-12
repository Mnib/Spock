#include <imgui/imgui.h>
#include <memory>

#include "example_layer.hh"
#include "images.hh"

void ExampleLayer::OnAttach() {
    m_Shapes = std::make_unique<ExampleShapes>();
    m_Image = std::make_unique<ExampleImage>();
}

void ExampleLayer::OnDetach() {
    // Free up memory
    m_Shapes = nullptr;
    m_Image = nullptr;
}

void ExampleLayer::OnUpdate(float delta_time) {
    static float rotation = 0;
    rotation += m_RotationSpeed * delta_time;

    // Update the shapes
    m_Shapes->Update(rotation);
    m_Image->Update(rotation);
}

void ExampleLayer::OnRender(VkCommandBuffer command_buffer) {
    // Render the shapes
    m_Shapes->Render(command_buffer);
    m_Image->Render(command_buffer);
}

void ExampleLayer::OnUIRender(VkCommandBuffer) {
    ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->Size.y - 100));
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Triangle Application", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Rotation speed", &m_RotationSpeed, 0, 5);

    ImGui::End();
}
