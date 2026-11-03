#pragma once

#include <imgui.h>
#include "monitors/CpuMonitor.hpp"
#include "monitors/MemoryMonitor.hpp"
#include "monitors/GpuMonitor.hpp"
#include "monitors/DiskMonitor.hpp"
#include "monitors/NetworkMonitor.hpp"

#include <chrono>

namespace SystemPulse::UI {

    class DashboardView {
    public:
        DashboardView(
            Monitors::CpuMonitor& cpuMon,
            Monitors::MemoryMonitor& memMon,
            Monitors::GpuMonitor& gpuMon,
            Monitors::DiskMonitor& diskMon,
            Monitors::NetworkMonitor& netMon
        );

        void Render();

        [[nodiscard]] float GetUpdateIntervalSeconds() const { return m_updateIntervalSec; }
        [[nodiscard]] bool IsPaused() const { return m_isPaused; }

    private:
        Monitors::CpuMonitor& m_cpu;
        Monitors::MemoryMonitor& m_memory;
        Monitors::GpuMonitor& m_gpu;
        Monitors::DiskMonitor& m_disk;
        Monitors::NetworkMonitor& m_network;

        std::chrono::steady_clock::time_point m_startTime;
        float m_updateIntervalSec{1.0f};
        bool m_isPaused{false};

        void RenderHeader();
        void RenderQuickKpiCards();
        void RenderTabs();

        void RenderOverviewTab();
        void RenderGpuTab();
        void RenderCpuTab();
        void RenderMemoryTab();
        void RenderDiskTab();
        void RenderNetworkTab();
        void RenderAboutTab();

        void RenderMetricCard(const char* title, float percentage, const char* subtitle, const char* extraInfo, const ImVec4& accentColor);
    };

} // namespace SystemPulse::UI

