#include "ui/DashboardView.hpp"
#include "ui/Theme.hpp"
#include "utils/Formatters.hpp"

#include <imgui.h>
#include <array>
#include <string>

namespace SystemPulse::UI {

    DashboardView::DashboardView(
        Monitors::CpuMonitor& cpuMon,
        Monitors::MemoryMonitor& memMon,
        Monitors::GpuMonitor& gpuMon,
        Monitors::DiskMonitor& diskMon,
        Monitors::NetworkMonitor& netMon
    )
        : m_cpu(cpuMon),
          m_memory(memMon),
          m_gpu(gpuMon),
          m_disk(diskMon),
          m_network(netMon),
          m_startTime(std::chrono::steady_clock::now())
    {}

    void DashboardView::Render() {
        // Main full-window docking host setup
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

        ImGui::Begin("SystemPulseMainWindow", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        RenderHeader();
        ImGui::Separator();
        ImGui::Spacing();

        RenderQuickKpiCards();
        ImGui::Spacing();

        RenderTabs();

        ImGui::End();
    }

    void DashboardView::RenderHeader() {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::AccentCyan);
        ImGui::TextUnformatted("SYSTEMPULSE");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Modern Hardware & Network Monitor");
        ImGui::EndGroup();

        ImGui::SameLine(ImGui::GetWindowWidth() - 480.0f);

        // Uptime counter
        auto now = std::chrono::steady_clock::now();
        uint64_t uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
        std::string uptimeStr = "Uptime: " + Utils::FormatUptime(uptimeSec);
        ImGui::TextDisabled("%s", uptimeStr.c_str());

        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        const char* intervals[] = { "0.5s", "1.0s", "2.0s" };
        int currentIdx = (m_updateIntervalSec == 0.5f) ? 0 : (m_updateIntervalSec == 1.0f ? 1 : 2);
        if (ImGui::Combo("##Interval", &currentIdx, intervals, IM_ARRAYSIZE(intervals))) {
            if (currentIdx == 0) m_updateIntervalSec = 0.5f;
            else if (currentIdx == 1) m_updateIntervalSec = 1.0f;
            else m_updateIntervalSec = 2.0f;
        }

        ImGui::SameLine();
        if (m_isPaused) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Colors::WarningAmber);
            if (ImGui::Button("Resume")) m_isPaused = false;
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("Pause")) m_isPaused = true;
        }
    }

    void DashboardView::RenderMetricCard(
        const char* title,
        float percentage,
        const char* subtitle,
        const char* extraInfo,
        const ImVec4& accentColor
    ) {
        ImVec2 cardSize = ImVec2(-1.0f, 95.0f);
        ImGui::BeginChild(title, cardSize, true, ImGuiWindowFlags_NoScrollbar);

        // Header with title and percentage
        ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 45.0f);
        ImVec4 statusColor = Theme::GetStatusColor(percentage);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::Text("%.1f%%", percentage);
        ImGui::PopStyleColor();

        // Progress bar
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, accentColor);
        ImGui::ProgressBar(percentage / 100.0f, ImVec2(-1.0f, 6.0f), "");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::TextUnformatted(subtitle);
        ImGui::TextDisabled("%s", extraInfo);

        ImGui::EndChild();
    }

    void DashboardView::RenderQuickKpiCards() {
        const auto& cpu = m_cpu.GetTelemetry();
        const auto& mem = m_memory.GetTelemetry();
        const auto& gpu = m_gpu.GetTelemetry();
        const auto& net = m_network.GetTelemetry();

        float cardWidth = (ImGui::GetContentRegionAvail().x - (3.0f * 12.0f)) / 4.0f;

        // 1. CPU Card
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        std::string cpuSub = Utils::FormatFrequency(cpu.clockSpeedMhz) + " | " + std::to_string(cpu.logicalCores) + " Cores";
        RenderMetricCard("CPU PROCESSOR", cpu.overallUsage, cpuSub.c_str(), cpu.modelName.c_str(), Theme::Colors::AccentCyan);
        ImGui::EndGroup();

        // 2. Memory Card
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        std::string memSub = Utils::FormatBytes(mem.usedPhysicalBytes) + " / " + Utils::FormatBytes(mem.totalPhysicalBytes);
        std::string memFree = "Available: " + Utils::FormatBytes(mem.availablePhysicalBytes);
        RenderMetricCard("MEMORY (RAM)", mem.usagePercentage, memSub.c_str(), memFree.c_str(), Theme::Colors::AccentEmerald);
        ImGui::EndGroup();

        // 3. GPU Card
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        std::string gpuSub = Utils::FormatBytes(gpu.usedMemoryBytes) + " / " + Utils::FormatBytes(gpu.dedicatedMemoryBytes);
        std::string gpuExtra = "Temp: " + std::to_string(static_cast<int>(gpu.temperatureCelsius)) + " C | " + gpu.name;
        RenderMetricCard("GRAPHICS (GPU)", gpu.coreUsagePercentage, gpuSub.c_str(), gpuExtra.c_str(), Theme::Colors::AccentViolet);
        ImGui::EndGroup();

        // 4. Network Card
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        std::string netSub = "DL: " + Utils::FormatSpeed(net.currentDownloadSpeed);
        std::string netExtra = "UL: " + Utils::FormatSpeed(net.currentUploadSpeed);
        float netPct = std::clamp((static_cast<float>(net.currentDownloadSpeed) / (100.0f * 1024.0f * 1024.0f)) * 100.0f, 2.0f, 100.0f);
        RenderMetricCard("NETWORK ADAPTER", netPct, netSub.c_str(), netExtra.c_str(), Theme::Colors::WarningAmber);
        ImGui::EndGroup();
    }

    void DashboardView::RenderTabs() {
        if (ImGui::BeginTabBar("MainMonitorTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Overview")) {
                RenderOverviewTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("GPU Engine")) {
                RenderGpuTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("CPU Core Matrix")) {
                RenderCpuTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Memory & Paging")) {
                RenderMemoryTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Drives & Storage")) {
                RenderDiskTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Network Traffic")) {
                RenderNetworkTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("System & Diagnostics")) {
                RenderAboutTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void DashboardView::RenderGpuTab() {
        const auto& gpu = m_gpu.GetTelemetry();

        ImGui::Spacing();

        // 1. Top Header Banner
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::AccentViolet);
        ImGui::Text("%s", gpu.name.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (gpu.isNvmlActive) {
            ImGui::TextColored(Theme::Colors::AccentEmerald, "[ NVIDIA NVML ACTIVE ]");
        } else {
            ImGui::TextColored(Theme::Colors::AccentCyan, "[ %s ]", gpu.apiBackend.c_str());
        }
        ImGui::EndGroup();

        // Temperature indicator badge
        // Green for <65°C, Amber for 65-80°C, Red for >80°C
        ImVec4 tempColor;
        const char* tempBadge;
        if (gpu.temperatureCelsius >= 80.0f) {
            tempColor = Theme::Colors::DangerRed;
            tempBadge = "HOT (>80 C)";
        } else if (gpu.temperatureCelsius >= 65.0f) {
            tempColor = Theme::Colors::WarningAmber;
            tempBadge = "WARM (65-80 C)";
        } else {
            tempColor = Theme::Colors::AccentEmerald;
            tempBadge = "NORMAL (<65 C)";
        }

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 330.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, tempColor);
        ImGui::Text("[ Temp: %.1f C  %s ]", gpu.temperatureCelsius, tempBadge);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (gpu.fanSpeedPercentage > 0) {
            ImGui::TextDisabled("[ Fan: %u%% ]", gpu.fanSpeedPercentage);
            ImGui::SameLine();
        }
        if (gpu.powerUsageWatts > 0.0f) {
            ImGui::TextDisabled("[ %.1f W ]", gpu.powerUsageWatts);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. VRAM Live Gauge Bar
        float vramUsedGB = static_cast<float>(gpu.usedMemoryBytes) / (1024.0f * 1024.0f * 1024.0f);
        float vramTotalGB = static_cast<float>(gpu.dedicatedMemoryBytes) / (1024.0f * 1024.0f * 1024.0f);
        float vramPct = (vramTotalGB > 0.0f) ? (vramUsedGB / vramTotalGB * 100.0f) : 0.0f;

        ImGui::Text("Dedicated Video Memory (VRAM): %.1f / %.1f GB (%.1f%%)", vramUsedGB, vramTotalGB, vramPct);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 160.0f);
        float vramFreeGB = std::max(0.0f, vramTotalGB - vramUsedGB);
        ImGui::TextDisabled("Available: %.1f GB", vramFreeGB);

        ImVec4 vramColor = Theme::GetStatusColor(vramPct);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, vramColor);
        char vramBarText[64];
        snprintf(vramBarText, sizeof(vramBarText), "%.1f / %.1f GB", vramUsedGB, vramTotalGB);
        ImGui::ProgressBar(vramPct / 100.0f, ImVec2(-1.0f, 20.0f), vramBarText);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // 3. Side-by-Side Scrolling History Graphs (Cyan / Purple Theme)
        float chartWidth = (ImGui::GetContentRegionAvail().x - 12.0f) / 2.0f;

        // Chart 1: GPU 3D Utilization (60s history)
        ImGui::BeginChild("GpuCoreChart", ImVec2(chartWidth, 180.0f), true);
        {
            std::array<float, 60> gpuPoints;
            gpu.usageHistory.CopyLinear(gpuPoints);

            ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::AccentViolet);
            ImGui::TextUnformatted("GPU 3D Engine Utilization (60s History)");
            ImGui::PopStyleColor();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::GetStatusColor(gpu.coreUsagePercentage));
            ImGui::Text("Live: %.1f%%", gpu.coreUsagePercentage);
            ImGui::PopStyleColor();

            char overlay[64];
            snprintf(overlay, sizeof(overlay), "Avg: %.1f%%  |  Peak: %.1f%%", gpu.usageHistory.Average(), gpu.usageHistory.Max());

            // Cyan / Purple Theme Plot
            ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::AccentViolet);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.10f, 0.15f, 1.00f));
            ImGui::PlotLines("##GpuLoadPlot", gpuPoints.data(), static_cast<int>(gpuPoints.size()), 0, overlay, 0.0f, 100.0f, ImVec2(-1.0f, 120.0f));
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();

        // Chart 2: Dedicated VRAM Allocation History
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginChild("GpuVramChart", ImVec2(chartWidth, 180.0f), true);
        {
            std::array<float, 60> vramPoints;
            gpu.vramHistory.CopyLinear(vramPoints);

            ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::AccentCyan);
            ImGui::TextUnformatted("VRAM Allocation History (60s Rolling)");
            ImGui::PopStyleColor();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::AccentCyan);
            ImGui::Text("Usage: %.1f%%", vramPct);
            ImGui::PopStyleColor();

            char vramOverlay[64];
            snprintf(vramOverlay, sizeof(vramOverlay), "Allocated: %.1f GB  |  Total: %.1f GB", vramUsedGB, vramTotalGB);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::AccentCyan);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.11f, 0.14f, 1.00f));
            ImGui::PlotLines("##GpuVramPlot", vramPoints.data(), static_cast<int>(vramPoints.size()), 0, vramOverlay, 0.0f, 100.0f, ImVec2(-1.0f, 120.0f));
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // 4. GPU Hardware Specifications Card
        ImGui::TextColored(Theme::Colors::AccentViolet, "GPU Hardware Specifications & Architecture:");
        ImGui::Spacing();

        if (ImGui::BeginTable("GpuSpecsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Specification");
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Property");
            ImGui::TableSetupColumn("Detail");
            ImGui::TableHeadersRow();

            // Row 1
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Graphics Card Model");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(Theme::Colors::AccentViolet, "%s", gpu.name.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("Driver Version");
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", gpu.driverVersion.c_str());

            // Row 2
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("DirectX Feature Level");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", gpu.directXVersion.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("PCIe Bus Location");
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", gpu.pcieBusId.c_str());

            // Row 3
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Dedicated Video Memory");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(gpu.dedicatedMemoryBytes).c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("Shared System Memory");
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", Utils::FormatBytes(gpu.sharedMemoryBytes).c_str());

            // Row 4
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Driver Telemetry API");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", gpu.apiBackend.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("Memory Controller Load");
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f%%", gpu.memoryControllerUsage);

            ImGui::EndTable();
        }
    }

    void DashboardView::RenderOverviewTab() {
        const auto& cpu = m_cpu.GetTelemetry();
        const auto& mem = m_memory.GetTelemetry();
        const auto& net = m_network.GetTelemetry();

        ImGui::Spacing();

        // CPU Graph
        std::array<float, 60> cpuPoints;
        cpu.usageHistory.CopyLinear(cpuPoints);
        std::string cpuHeader = "CPU Utilization (60s History) - Current: " + Utils::FormatPercentage(cpu.overallUsage);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::AccentCyan);
        ImGui::PlotLines("##CpuHistory", cpuPoints.data(), static_cast<int>(cpuPoints.size()), 0, cpuHeader.c_str(), 0.0f, 100.0f, ImVec2(-1.0f, 100.0f));
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // RAM Graph
        std::array<float, 60> memPoints;
        mem.usageHistory.CopyLinear(memPoints);
        std::string memHeader = "Memory Usage (60s History) - Current: " + Utils::FormatPercentage(mem.usagePercentage);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::AccentEmerald);
        ImGui::PlotLines("##MemHistory", memPoints.data(), static_cast<int>(memPoints.size()), 0, memHeader.c_str(), 0.0f, 100.0f, ImVec2(-1.0f, 100.0f));
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // GPU Graph
        const auto& gpu = m_gpu.GetTelemetry();
        std::array<float, 60> gpuPoints;
        gpu.usageHistory.CopyLinear(gpuPoints);
        std::string gpuHeader = "GPU 3D Engine Utilization (60s History) - Current: " + Utils::FormatPercentage(gpu.coreUsagePercentage) + " (" + gpu.name + ")";
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::AccentViolet);
        ImGui::PlotLines("##GpuHistory", gpuPoints.data(), static_cast<int>(gpuPoints.size()), 0, gpuHeader.c_str(), 0.0f, 100.0f, ImVec2(-1.0f, 100.0f));
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Network Download Throughput Graph
        std::array<float, 60> netPoints;
        net.downloadHistory.CopyLinear(netPoints);
        float maxNet = std::max(net.downloadHistory.Max() * 1.2f, 500.0f);
        std::string netHeader = "Network Download Rate (KB/s) - Peak: " + std::to_string(static_cast<int>(net.downloadHistory.Max())) + " KB/s";
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::Colors::WarningAmber);
        ImGui::PlotLines("##NetHistory", netPoints.data(), static_cast<int>(netPoints.size()), 0, netHeader.c_str(), 0.0f, maxNet, ImVec2(-1.0f, 100.0f));
        ImGui::PopStyleColor();
    }

    void DashboardView::RenderCpuTab() {
        const auto& cpu = m_cpu.GetTelemetry();

        ImGui::Spacing();
        ImGui::Text("Model: %s", cpu.modelName.c_str());
        ImGui::Text("Architecture: x86_64 | Logical Cores: %u | Physical Cores: %u", cpu.logicalCores, cpu.physicalCores);
        ImGui::Text("Frequency: %s | Active Threads: %u", Utils::FormatFrequency(cpu.clockSpeedMhz).c_str(), cpu.activeThreads);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Per-Core Utilization Grid:");
        ImGui::Spacing();

        int columns = (cpu.logicalCores > 16) ? 8 : 4;
        if (ImGui::BeginTable("CpuCoresTable", columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            for (int col = 0; col < columns; ++col) {
                ImGui::TableSetupColumn(("Core Batch " + std::to_string(col + 1)).c_str());
            }

            for (size_t i = 0; i < cpu.perCoreUsage.size(); ++i) {
                if (i % columns == 0) {
                    ImGui::TableNextRow();
                }
                ImGui::TableSetColumnIndex(static_cast<int>(i % columns));

                float usage = cpu.perCoreUsage[i];
                ImGui::Text("Core #%02zu: %.1f%%", i, usage);
                ImVec4 coreColor = Theme::GetStatusColor(usage);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, coreColor);
                ImGui::ProgressBar(usage / 100.0f, ImVec2(-1.0f, 8.0f), "");
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }
    }

    void DashboardView::RenderMemoryTab() {
        const auto& mem = m_memory.GetTelemetry();

        ImGui::Spacing();
        ImGui::TextUnformatted("Physical Memory Overview:");
        ImGui::ProgressBar(mem.usagePercentage / 100.0f, ImVec2(-1.0f, 22.0f), Utils::FormatPercentage(mem.usagePercentage).c_str());

        ImGui::Spacing();
        if (ImGui::BeginTable("MemoryStatsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Total Installed RAM");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(mem.totalPhysicalBytes).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Used Physical RAM");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(mem.usedPhysicalBytes).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Available Physical RAM");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(mem.availablePhysicalBytes).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Total Page File / Virtual");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(mem.totalPageFileBytes).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Used Page File");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", Utils::FormatBytes(mem.usedPageFileBytes).c_str());

            ImGui::EndTable();
        }
    }

    void DashboardView::RenderDiskTab() {
        const auto& disk = m_disk.GetTelemetry();

        ImGui::Spacing();
        ImGui::Text("Real-Time I/O: Read %s | Write %s",
            Utils::FormatSpeed(disk.totalReadRateBytes).c_str(),
            Utils::FormatSpeed(disk.totalWriteRateBytes).c_str()
        );
        ImGui::Spacing();

        for (const auto& drive : disk.drives) {
            std::string cardId = "Drive_" + drive.mountPoint;
            ImGui::BeginChild(cardId.c_str(), ImVec2(-1.0f, 90.0f), true);

            ImGui::Text("%s (%s) [%s]", drive.mountPoint.c_str(),
                drive.volumeName.empty() ? "Local Disk" : drive.volumeName.c_str(),
                drive.fileSystem.c_str());

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
            ImGui::Text("%.1f%%", drive.usagePercentage);

            ImVec4 driveColor = Theme::GetStatusColor(drive.usagePercentage);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, driveColor);
            ImGui::ProgressBar(drive.usagePercentage / 100.0f, ImVec2(-1.0f, 10.0f), "");
            ImGui::PopStyleColor();

            ImGui::TextDisabled("Used: %s / Total: %s (Free: %s)",
                Utils::FormatBytes(drive.usedBytes).c_str(),
                Utils::FormatBytes(drive.totalBytes).c_str(),
                Utils::FormatBytes(drive.freeBytes).c_str()
            );

            ImGui::EndChild();
            ImGui::Spacing();
        }
    }

    void DashboardView::RenderNetworkTab() {
        const auto& net = m_network.GetTelemetry();

        ImGui::Spacing();
        ImGui::Text("Total Received: %s | Total Transmitted: %s",
            Utils::FormatBytes(net.totalBytesReceived).c_str(),
            Utils::FormatBytes(net.totalBytesSent).c_str()
        );
        ImGui::Spacing();

        ImGui::TextUnformatted("Active Physical & Primary Virtual Network Interfaces:");
        if (ImGui::BeginTable("NetAdaptersTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Adapter Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Description");
            ImGui::TableSetupColumn("IP Address");
            ImGui::TableSetupColumn("MAC Address");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Throughput In/Out");
            ImGui::TableHeadersRow();

            for (const auto& adapter : net.adapters) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(adapter.name.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::TextColored(Theme::Colors::AccentCyan, "%s", adapter.type.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(adapter.description.c_str());
                ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(adapter.ipAddress.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::TextDisabled("%s", adapter.macAddress.c_str());
                ImGui::TableSetColumnIndex(5);
                if (adapter.isOperational) {
                    ImGui::TextColored(Theme::Colors::AccentEmerald, "CONNECTED");
                } else {
                    ImGui::TextDisabled("DISCONNECTED");
                }
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("In: %s | Out: %s",
                    Utils::FormatBytes(adapter.bytesReceived).c_str(),
                    Utils::FormatBytes(adapter.bytesSent).c_str()
                );
            }
            ImGui::EndTable();
        }
    }

    void DashboardView::RenderAboutTab() {
        ImGui::Spacing();
        ImGui::TextColored(Theme::Colors::AccentCyan, "SystemPulse v1.0.0");
        ImGui::TextUnformatted("Modern Real-Time System Hardware & Performance Monitor");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BulletText("Standard: C++20");
        ImGui::BulletText("UI Framework: Dear ImGui %s (Docking Branch)", IMGUI_VERSION);
        ImGui::BulletText("Window / Context: GLFW 3.4 & OpenGL 3.3 Core");
        ImGui::BulletText("Platform: Windows 64-bit (DirectX DXGI, Windows PDH, IP Helper)");
        ImGui::BulletText("License: MIT");
        ImGui::Spacing();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application Performance: %.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / (io.Framerate > 0.0f ? io.Framerate : 60.0f));
    }

} // namespace SystemPulse::UI

