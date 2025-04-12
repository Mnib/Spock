#include <backends/imgui_impl_glfw.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

#include "spock/spock.hh"
#include "spock/vulkan.hh"
#include "spock/window.hh"

namespace spock
{
    static void check_vk_result(VkResult err) {
        if (err == VK_SUCCESS)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
        if (err < 0)
            abort();
    }

    static ImVec4 SrgbToLinear(const ImVec4 &c) {
        auto convert = [](float channel) {
            return (channel <= 0.04045f) ? (channel / 12.92f) : powf((channel + 0.055f) / 1.055f, 2.4f);
        };

        return ImVec4(convert(c.x), convert(c.y), convert(c.z), c.w); // leave alpha as-is
    }

    static void ConvertImGuiStyleToLinear(ImGuiStyle &style) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            style.Colors[i] = SrgbToLinear(style.Colors[i]);
        }
    }

    static void SetupTheme(ImGuiStyle &style) {
        ImVec4 *colors = style.Colors;

        auto black_0 = ImVec4(0.050980392156862744f, 0.047058823529411764f, 0.047058823529411764f, 1.f);
        auto black_2 = ImVec4(0.11372549019607843f, 0.10980392156862745f, 0.09803921568627451f, 1.f);
        auto black_3 = ImVec4(0.09411764705882353f, 0.08627450980392157f, 0.08627450980392157f, 1.f);
        auto black_5 = ImVec4(0.1568627450980392f, 0.15294117647058825f, 0.15294117647058825f, 1.f);

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_WindowBg] = black_3;
        colors[ImGuiCol_TitleBg] = black_0;
        colors[ImGuiCol_TitleBgActive] = black_5;
        colors[ImGuiCol_MenuBarBg] = black_2;

        // sRGB to Linear colors (Vulkan performs the sRGB conversion)
        ConvertImGuiStyleToLinear(style);

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(5, 5);
        style.ItemSpacing = ImVec2(10, 5);
        style.ItemInnerSpacing = ImVec2(5, 5);
        style.TouchExtraPadding = ImVec2(0, 0);
    }

    void Spock::InitImGUI() {
        // Setup ImGUI
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

        io.IniFilename = nullptr; // Disable layout saving

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        SetupTheme(style);

        auto queue_families = FindQueueFamilies(s_VulkanContext.PhysicalDevice, s_VulkanContext.Surface);

        ImGui_ImplGlfw_InitForVulkan(s_VulkanContext.Win->GetWindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = s_VulkanContext.Instance;
        init_info.PhysicalDevice = s_VulkanContext.PhysicalDevice;
        init_info.Device = s_VulkanContext.Device;
        init_info.QueueFamily = *queue_families.GraphicsFamily;
        init_info.Queue = s_VulkanContext.GraphicsQueue;
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = s_VulkanContext.DescriptorPool;
        init_info.RenderPass = s_VulkanContext.RenderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
        init_info.MSAASamples = s_VulkanContext.MaxUsableSamples;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);
    }

    void Spock::CleanupImGUI() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();
    }
} // namespace spock
