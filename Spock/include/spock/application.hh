#pragma once

#include <memory>
#include <vector>

namespace spock
{
    class Layer;
    struct SpockSettings;

    class Application {
      public:
        Application(const SpockSettings &vulkan_settings);
        Application(const Application &) = delete;
        Application operator=(const Application &) = delete;
        ~Application();

        void Run();

        void PushLayer(const std::shared_ptr<Layer> layer);

      private:
        std::vector<std::shared_ptr<Layer>> m_Layers;
    };
} // namespace spock
