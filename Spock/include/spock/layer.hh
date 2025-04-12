#pragma once

#include <vulkan/vulkan_core.h>

namespace spock
{
    class VulkanInstance;
    class Application;

    class Layer {
      public:
        Layer(Application &application);

        virtual ~Layer() = default;
        virtual void OnAttach() = 0;
        virtual void OnDetach() = 0;
        virtual void OnUpdate(float delta_time) = 0;
        virtual void OnRender(VkCommandBuffer command_buffer) = 0;
        virtual void OnUIRender(VkCommandBuffer command_buffer) = 0;

      protected:
        Application &m_Application;
    };
} // namespace spock
