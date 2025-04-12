#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "images.hh"
#include "shapes.hh"
#include "spock/layer.hh"

namespace spock
{
    class Application;
} // namespace spock

class ExampleLayer : public spock::Layer {
  public:
    ExampleLayer(spock::Application &application)
        : spock::Layer(application) {
    }

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnRender(VkCommandBuffer command_buffer) override;
    virtual void OnUIRender(VkCommandBuffer command_buffer) override;

  private:
    void CreateDescriptorSets();

  private:
    std::unique_ptr<ExampleShapes> m_Shapes;
    std::unique_ptr<ExampleImage> m_Image;
    float m_RotationSpeed = 1.f;
};
