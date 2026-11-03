#include "core/Application.hpp"
#include "ui/Theme.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>

namespace SystemPulse::Core {

    Application::Application(const std::string& title, int width, int height)
        : m_window(WindowConfig{ title, width, height, true }),
          m_dashboard(m_cpuMonitor, m_memoryMonitor, m_gpuMonitor, m_diskMonitor, m_networkMonitor)
    {
        InitializeImGui();

        // Initialize all hardware telemetry monitors
        m_cpuMonitor.Initialize();
        m_memoryMonitor.Initialize();
        m_gpuMonitor.Initialize();
        m_diskMonitor.Initialize();
        m_networkMonitor.Initialize();
    }

    Application::~Application() {
        ShutdownImGui();
    }

    void Application::InitializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Apply our custom modern dark styling
        UI::Theme::ApplyDarkTheme();

        // When viewports are enabled, tweak WindowRounding/WindowBg for platform windows
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window.GetNativeHandle(), true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void Application::ShutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Application::UpdateMonitors() {
        m_cpuMonitor.Update();
        m_memoryMonitor.Update();
        m_gpuMonitor.Update();
        m_diskMonitor.Update();
        m_networkMonitor.Update();
    }

    void Application::Run() {
        auto lastMonitorUpdate = std::chrono::steady_clock::now();

        while (!m_window.ShouldClose()) {
            m_window.PollEvents();

            // Periodic telemetry polling timer
            auto now = std::chrono::steady_clock::now();
            float elapsedSec = std::chrono::duration<float>(now - lastMonitorUpdate).count();
            if (!m_dashboard.IsPaused() && elapsedSec >= m_dashboard.GetUpdateIntervalSeconds()) {
                UpdateMonitors();
                lastMonitorUpdate = now;
            }

            // Start Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Render Dashboard View
            m_dashboard.Render();

            // Finalize ImGui rendering
            ImGui::Render();

            int displayW = 0;
            int displayH = 0;
            glfwGetFramebufferSize(m_window.GetNativeHandle(), &displayW, &displayH);
            glViewport(0, 0, displayW, displayH);
            glClearColor(0.07f, 0.08f, 0.11f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows (when multi-viewports enabled)
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backupCurrentContext);
            }

            m_window.SwapBuffers();
        }
    }

} // namespace SystemPulse::Core

