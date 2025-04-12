#include <memory>
#include <vulkan/vulkan_core.h>

#include "spock/application.hh"

#include "example_layer.hh"

int main() {
    auto settings = spock::SpockSettings{};             // Get the default settings
    settings.PresentModes = {VK_PRESENT_MODE_FIFO_KHR}; // Set present mode to V-Sync

    spock::Application app(std::move(settings));

    app.PushLayer(std::make_shared<ExampleLayer>(app));

    app.Run();

    return 0;
}
