#pragma once

#include "core/Window.hpp"
#include "monitors/CpuMonitor.hpp"
#include "monitors/MemoryMonitor.hpp"
#include "monitors/GpuMonitor.hpp"
#include "monitors/DiskMonitor.hpp"
#include "monitors/NetworkMonitor.hpp"
#include "ui/DashboardView.hpp"

#include <memory>
#include <string>

namespace SystemPulse::Core {

    class Application {
    public:
        explicit Application(const std::string& title = "SystemPulse - Hardware Monitor", int width = 1280, int height = 800);
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        void Run();

    private:
        Window m_window;

        // Telemetry Monitoring Subsystems
        Monitors::CpuMonitor m_cpuMonitor;
        Monitors::MemoryMonitor m_memoryMonitor;
        Monitors::GpuMonitor m_gpuMonitor;
        Monitors::DiskMonitor m_diskMonitor;
        Monitors::NetworkMonitor m_networkMonitor;

        // UI View
        UI::DashboardView m_dashboard;

        void InitializeImGui();
        void ShutdownImGui();
        void UpdateMonitors();
    };

} // namespace SystemPulse::Core

