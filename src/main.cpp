#include "SystemMetrics.hpp"
#include "utils/EnvLoader.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace {

    // ---------------------------------------------------------------------------
    // Color Palette & Status Indicator Helpers
    // ---------------------------------------------------------------------------
    namespace Palette {
        constexpr ImVec4 Background        = ImVec4(0.06f, 0.07f, 0.09f, 1.00f);
        constexpr ImVec4 TitleBar          = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
        constexpr ImVec4 CardBg            = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        constexpr ImVec4 CardBorder        = ImVec4(0.18f, 0.21f, 0.28f, 0.80f);
        constexpr ImVec4 FrameBg           = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);

        // Accent Colors
        constexpr ImVec4 Cyan              = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
        constexpr ImVec4 Violet            = ImVec4(0.55f, 0.40f, 1.00f, 1.00f);

        // Smooth Color-Coded Indicators
        constexpr ImVec4 NormalGreen       = ImVec4(0.00f, 0.90f, 0.46f, 1.00f); // Load < 70%
        constexpr ImVec4 AmberWarning      = ImVec4(1.00f, 0.70f, 0.00f, 1.00f); // Load 70% - 90%
        constexpr ImVec4 CriticalRed       = ImVec4(1.00f, 0.25f, 0.25f, 1.00f); // Load > 90%

        // Typography
        constexpr ImVec4 TextPrimary       = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
        constexpr ImVec4 TextSecondary     = ImVec4(0.62f, 0.67f, 0.76f, 1.00f);
        constexpr ImVec4 TextMuted         = ImVec4(0.40f, 0.44f, 0.52f, 1.00f);
    }

    // Smooth color-coded indicator logic:
    // Green: Normal (< 70%)
    // Amber: Warning (>= 70% and < 90%)
    // Red: Critical (>= 90%)
    ImVec4 GetLoadColor(float percentage) {
        if (percentage >= 90.0f) {
            return Palette::CriticalRed;
        } else if (percentage >= 70.0f) {
            return Palette::AmberWarning;
        } else {
            return Palette::NormalGreen;
        }
    }

    const char* GetLoadStatusText(float percentage) {
        if (percentage >= 90.0f) return "CRITICAL";
        if (percentage >= 70.0f) return "HIGH LOAD";
        return "OPTIMAL";
    }

    // ---------------------------------------------------------------------------
    // Custom Borderless Window Dragging State
    // ---------------------------------------------------------------------------
    struct WindowDragState {
        bool isDragging{false};
#ifdef _WIN32
        POINT startCursor{};
        int startWinX{0};
        int startWinY{0};
#else
        double startCursorX{0.0};
        double startCursorY{0.0};
        int startWinX{0};
        int startWinY{0};
#endif
    } g_dragState;

    void ApplyCustomDarkTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                  = Palette::TextPrimary;
        colors[ImGuiCol_TextDisabled]          = Palette::TextMuted;
        colors[ImGuiCol_WindowBg]              = Palette::Background;
        colors[ImGuiCol_ChildBg]               = Palette::CardBg;
        colors[ImGuiCol_PopupBg]               = ImVec4(0.09f, 0.11f, 0.15f, 0.98f);
        colors[ImGuiCol_Border]                = Palette::CardBorder;
        colors[ImGuiCol_FrameBg]               = Palette::FrameBg;
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.22f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.28f, 0.38f, 1.00f);
        colors[ImGuiCol_TitleBg]               = Palette::TitleBar;
        colors[ImGuiCol_TitleBgActive]         = Palette::TitleBar;
        colors[ImGuiCol_Button]                = ImVec4(0.16f, 0.19f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.23f, 0.28f, 0.38f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.28f, 0.34f, 0.46f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.22f, 0.30f, 0.70f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.30f, 0.40f, 0.85f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.35f, 0.48f, 1.00f);
        colors[ImGuiCol_Separator]             = Palette::CardBorder;
        colors[ImGuiCol_PlotLines]             = Palette::Cyan;
        colors[ImGuiCol_PlotLinesHovered]      = Palette::AmberWarning;
        colors[ImGuiCol_PlotHistogram]         = Palette::NormalGreen;
        colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.24f, 0.33f, 1.00f);
        colors[ImGuiCol_TabSelected]           = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);

        style.WindowRounding    = 8.0f;
        style.ChildRounding     = 6.0f;
        style.FrameRounding     = 5.0f;
        style.PopupRounding     = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 6.0f;

        style.WindowPadding     = ImVec2(16.0f, 16.0f);
        style.FramePadding      = ImVec2(10.0f, 6.0f);
        style.ItemSpacing       = ImVec2(10.0f, 10.0f);
        style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f;
    }

    // ---------------------------------------------------------------------------
    // UI Component: Custom Title Bar with Window Controls
    // ---------------------------------------------------------------------------
    void RenderCustomTitleBar(GLFWwindow* window, float systemMaxLoad) {
        float titleBarHeight = 36.0f;
        ImVec2 curPos = ImGui::GetCursorPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        // Title bar background canvas
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetWindowPos();
        ImVec2 pMax = ImVec2(pMin.x + windowSize.x, pMin.y + titleBarHeight);
        drawList->AddRectFilled(pMin, pMax, ImGui::GetColorU32(Palette::TitleBar));
        drawList->AddLine(ImVec2(pMin.x, pMax.y), pMax, ImGui::GetColorU32(Palette::CardBorder), 1.0f);

        // Invisible drag zone across title bar (excluding button area)
        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
        ImGui::InvisibleButton("##TitleBarDragZone", ImVec2(windowSize.x - 120.0f, titleBarHeight));

        // Dragging handler
#ifdef _WIN32
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            g_dragState.isDragging = true;
            GetCursorPos(&g_dragState.startCursor);
            glfwGetWindowPos(window, &g_dragState.startWinX, &g_dragState.startWinY);
        }
        if (g_dragState.isDragging) {
            if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0) {
                POINT currentCursor;
                GetCursorPos(&currentCursor);
                int deltaX = currentCursor.x - g_dragState.startCursor.x;
                int deltaY = currentCursor.y - g_dragState.startCursor.y;
                glfwSetWindowPos(window, g_dragState.startWinX + deltaX, g_dragState.startWinY + deltaY);
            } else {
                g_dragState.isDragging = false;
            }
        }
#else
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            int wx, wy;
            glfwGetWindowPos(window, &wx, &wy);
            glfwSetWindowPos(window, wx + static_cast<int>(delta.x), wy + static_cast<int>(delta.y));
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
#endif

        // Title Bar Content: Brand Logo & Title
        ImGui::SetCursorPos(ImVec2(14.0f, 8.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, Palette::Cyan);
        ImGui::TextUnformatted("[*]");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextUnformatted("SYSTEMPULSE");
        ImGui::SameLine();
        ImGui::TextDisabled("| Performance & Hardware Monitor");

        // Status Indicator Pill (Smooth Color-Coded)
        ImVec4 statusColor = GetLoadColor(systemMaxLoad);
        const char* statusText = GetLoadStatusText(systemMaxLoad);

        ImGui::SameLine(windowSize.x - 280.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::Text("(*) %s", statusText);
        ImGui::PopStyleColor();

        // Window Control Buttons (Minimize, Maximize, Close)
        ImGui::SetCursorPos(ImVec2(windowSize.x - 110.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        // 1. Minimize Button
        if (ImGui::Button("-", ImVec2(30.0f, 24.0f))) {
            glfwIconifyWindow(window);
        }

        // 2. Maximize / Restore Button
        ImGui::SameLine(0.0f, 4.0f);
        bool isMaximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
        if (ImGui::Button(isMaximized ? "[]" : "[+]", ImVec2(30.0f, 24.0f))) {
            if (isMaximized) {
                glfwRestoreWindow(window);
            } else {
                glfwMaximizeWindow(window);
            }
        }

        // 3. Close Button
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.00f));
        if (ImGui::Button("X", ImVec2(32.0f, 24.0f))) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        ImGui::PopStyleColor(2);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // Restore cursor position for main content below title bar
        ImGui::SetCursorPos(ImVec2(curPos.x, curPos.y + titleBarHeight + 6.0f));
    }

    // ---------------------------------------------------------------------------
    // UI Component: Metric Card
    // ---------------------------------------------------------------------------
    void RenderMetricCard(
        const char* title,
        float percentage,
        const char* primaryValue,
        const char* secondaryValue,
        const ImVec4& accentColor
    ) {
        ImVec4 statusColor = GetLoadColor(percentage);
        const char* statusBadge = GetLoadStatusText(percentage);

        ImGui::BeginChild(title, ImVec2(-1.0f, 120.0f), true, ImGuiWindowFlags_NoScrollbar);

        // Header: Title & Indicator Pill
        ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();

        // Right-aligned smooth status badge
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::Text("[ %s ]", statusBadge);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Large Primary Metric Display
        ImGui::PushStyleColor(ImGuiCol_Text, Palette::TextPrimary);
        ImGui::Text("%s", primaryValue);
        ImGui::PopStyleColor();

        // Smooth Color-Coded Progress Bar
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, statusColor);
        ImGui::ProgressBar(percentage / 100.0f, ImVec2(-1.0f, 6.0f), "");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Secondary Details
        ImGui::TextDisabled("%s", secondaryValue);

        ImGui::EndChild();
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Main Application Entry Point
// ---------------------------------------------------------------------------
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Load local environment configuration (.env) if present
    SystemPulse::Utils::EnvLoader::Load();

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    // Modern OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Borderless Window Configuration (Custom Styled Frame)
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    int windowWidth = SystemPulse::Utils::EnvLoader::GetInt("SYSTEMPULSE_WINDOW_WIDTH", 1200);
    int windowHeight = SystemPulse::Utils::EnvLoader::GetInt("SYSTEMPULSE_WINDOW_HEIGHT", 780);
    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "SystemPulse - Modern Hardware & Performance Monitor",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply custom modern dark cyberpunk theme
    ApplyCustomDarkTheme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize Cross-Platform Telemetry Collector
    SystemPulse::SystemMetrics metrics;
    metrics.Initialize();

    // 60-Second Rolling History Buffers for Real-Time Charts
    constexpr size_t HISTORY_POINTS = 60;
    std::array<float, HISTORY_POINTS> cpuHistory{};
    std::array<float, HISTORY_POINTS> ramHistory{};
    cpuHistory.fill(0.0f);
    ramHistory.fill(0.0f);

    auto lastSampleTime = std::chrono::steady_clock::now();
    auto appStartTime = std::chrono::steady_clock::now();
    float refreshIntervalSec = SystemPulse::Utils::EnvLoader::GetFloat("SYSTEMPULSE_REFRESH_INTERVAL_SEC", 1.0f);
    bool isPaused = false;

    // Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Periodic Telemetry Polling
        auto now = std::chrono::steady_clock::now();
        float elapsedSample = std::chrono::duration<float>(now - lastSampleTime).count();

        if (!isPaused && elapsedSample >= refreshIntervalSec) {
            metrics.Update();
            lastSampleTime = now;

            const auto& snap = metrics.GetSnapshot();

            // Shift and push newest values for real-time scrolling history
            for (size_t i = 0; i < HISTORY_POINTS - 1; ++i) {
                cpuHistory[i] = cpuHistory[i + 1];
                ramHistory[i] = ramHistory[i + 1];
            }
            cpuHistory[HISTORY_POINTS - 1] = snap.cpu.usagePercent;
            ramHistory[HISTORY_POINTS - 1] = snap.memory.usagePercent;
        }

        const auto& snap = metrics.GetSnapshot();
        float systemMaxLoad = std::max(snap.cpu.usagePercent, snap.memory.usagePercent);

        // Start Dear ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Borderless Main Fullscreen Canvas
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                                     ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

        ImGui::Begin("SystemPulseRootWindow", nullptr, rootFlags);
        ImGui::PopStyleVar(3);

        // 1. Render Custom Draggable Title Bar with Controls
        RenderCustomTitleBar(window, systemMaxLoad);

        // 2. Control & Utility Bar
        auto totalUptime = std::chrono::duration_cast<std::chrono::seconds>(now - appStartTime).count();
        uint64_t uptimeM = totalUptime / 60;
        uint64_t uptimeS = totalUptime % 60;
        std::string uptimeStr = "Uptime: " + std::to_string(uptimeM) + "m " + std::to_string(uptimeS) + "s";

        ImGui::TextDisabled("%s", uptimeStr.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("| Target FPS: 60 | Frame Time: %.2f ms", 1000.0f / (io.Framerate > 0.0f ? io.Framerate : 60.0f));

        ImGui::SameLine(ImGui::GetWindowWidth() - 260.0f);
        ImGui::SetNextItemWidth(90.0f);
        const char* intervals[] = { "0.5s", "1.0s", "2.0s" };
        int intervalIdx = (refreshIntervalSec == 0.5f) ? 0 : (refreshIntervalSec == 1.0f ? 1 : 2);
        if (ImGui::Combo("##Interval", &intervalIdx, intervals, IM_ARRAYSIZE(intervals))) {
            if (intervalIdx == 0) refreshIntervalSec = 0.5f;
            else if (intervalIdx == 1) refreshIntervalSec = 1.0f;
            else refreshIntervalSec = 2.0f;
        }

        ImGui::SameLine();
        if (isPaused) {
            ImGui::PushStyleColor(ImGuiCol_Button, Palette::AmberWarning);
            if (ImGui::Button("Resume")) isPaused = false;
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("Pause")) isPaused = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 3. Four Sleek Metric Cards (CPU, RAM, Disk, Network)
        float totalWidth = ImGui::GetContentRegionAvail().x;
        float cardWidth = (totalWidth - (3.0f * 12.0f)) / 4.0f;

        // Card 1: CPU Utilization
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        char cpuPrimary[32];
        snprintf(cpuPrimary, sizeof(cpuPrimary), "%.1f%% Load", snap.cpu.usagePercent);
        RenderMetricCard("CPU UTILIZATION", snap.cpu.usagePercent, cpuPrimary, "Multi-Threaded Hardware Engine", Palette::Cyan);
        ImGui::EndGroup();

        // Card 2: Memory (RAM)
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        char ramPrimary[48];
        snprintf(ramPrimary, sizeof(ramPrimary), "%.2f / %.2f GB", snap.memory.usedGB, snap.memory.totalGB);
        char ramSecondary[48];
        snprintf(ramSecondary, sizeof(ramSecondary), "Available: %.2f GB (%.1f%%)", snap.memory.availableGB, snap.memory.usagePercent);
        RenderMetricCard("MEMORY (RAM)", snap.memory.usagePercent, ramPrimary, ramSecondary, Palette::NormalGreen);
        ImGui::EndGroup();

        // Card 3: Disk Activity
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        char diskPrimary[64];
        snprintf(diskPrimary, sizeof(diskPrimary), "R: %.1f MB/s | W: %.1f MB/s", snap.disk.readMBPerSec, snap.disk.writeMBPerSec);
        char diskSecondary[64];
        snprintf(diskSecondary, sizeof(diskSecondary), "Total Read: %s", SystemPulse::SystemMetrics::FormatBytes(snap.disk.totalBytesRead).c_str());
        float diskPct = std::clamp(static_cast<float>((snap.disk.readMBPerSec + snap.disk.writeMBPerSec) / 100.0 * 100.0), 3.0f, 100.0f);
        RenderMetricCard("DISK I/O ACTIVITY", diskPct, diskPrimary, diskSecondary, Palette::Violet);
        ImGui::EndGroup();

        // Card 4: Network Bandwidth
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginGroup();
        ImGui::PushItemWidth(cardWidth);
        char netPrimary[64];
        snprintf(netPrimary, sizeof(netPrimary), "DL: %.2f | UL: %.2f MB/s", snap.network.downloadMBPerSec, snap.network.uploadMBPerSec);
        char netSecondary[64];
        snprintf(netSecondary, sizeof(netSecondary), "Recv: %s", SystemPulse::SystemMetrics::FormatBytes(snap.network.totalBytesReceived).c_str());
        float netPct = std::clamp(static_cast<float>((snap.network.downloadMBPerSec / 50.0) * 100.0), 3.0f, 100.0f);
        RenderMetricCard("NETWORK BANDWIDTH", netPct, netPrimary, netSecondary, Palette::AmberWarning);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 4. Real-Time Scrolling History Graphs (CPU and RAM)
        ImGui::TextColored(Palette::Cyan, "REAL-TIME PERFORMANCE TELEMETRY");
        ImGui::SameLine();
        ImGui::TextDisabled("(60s Rolling Buffer)");

        ImGui::Spacing();

        float chartWidth = (ImGui::GetContentRegionAvail().x - 12.0f) / 2.0f;

        // Chart A: CPU Utilization Scrolling Graph
        ImGui::BeginChild("CpuGraphPanel", ImVec2(chartWidth, 190.0f), true);
        {
            ImVec4 cpuColor = GetLoadColor(snap.cpu.usagePercent);
            float cpuAvg = 0.0f;
            float cpuMax = 0.0f;
            for (float v : cpuHistory) {
                cpuAvg += v;
                if (v > cpuMax) cpuMax = v;
            }
            cpuAvg /= static_cast<float>(cpuHistory.size());

            ImGui::PushStyleColor(ImGuiCol_Text, Palette::Cyan);
            ImGui::TextUnformatted("CPU Load History");
            ImGui::PopStyleColor();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, cpuColor);
            ImGui::Text("Current: %.1f%%", snap.cpu.usagePercent);
            ImGui::PopStyleColor();

            char cpuOverlay[64];
            snprintf(cpuOverlay, sizeof(cpuOverlay), "Avg: %.1f%%  |  Peak: %.1f%%", cpuAvg, cpuMax);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, cpuColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.09f, 0.12f, 1.0f));
            ImGui::PlotLines(
                "##CpuPlot",
                cpuHistory.data(),
                static_cast<int>(cpuHistory.size()),
                0,
                cpuOverlay,
                0.0f,
                100.0f,
                ImVec2(-1.0f, 125.0f)
            );
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();

        // Chart B: Memory (RAM) Scrolling Graph
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::BeginChild("RamGraphPanel", ImVec2(chartWidth, 190.0f), true);
        {
            ImVec4 ramColor = GetLoadColor(snap.memory.usagePercent);
            float ramAvg = 0.0f;
            float ramMax = 0.0f;
            for (float v : ramHistory) {
                ramAvg += v;
                if (v > ramMax) ramMax = v;
            }
            ramAvg /= static_cast<float>(ramHistory.size());

            ImGui::PushStyleColor(ImGuiCol_Text, Palette::NormalGreen);
            ImGui::TextUnformatted("Memory Utilization History");
            ImGui::PopStyleColor();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 150.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ramColor);
            ImGui::Text("Used: %.2f GB (%.1f%%)", snap.memory.usedGB, snap.memory.usagePercent);
            ImGui::PopStyleColor();

            char ramOverlay[64];
            snprintf(ramOverlay, sizeof(ramOverlay), "Avg: %.1f%%  |  Peak: %.1f%%", ramAvg, ramMax);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ramColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.09f, 0.12f, 1.0f));
            ImGui::PlotLines(
                "##RamPlot",
                ramHistory.data(),
                static_cast<int>(ramHistory.size()),
                0,
                ramOverlay,
                0.0f,
                100.0f,
                ImVec2(-1.0f, 125.0f)
            );
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // 5. Bottom Status Bar with OpenGL & Engine Diagnostics
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(Palette::Cyan, "OpenGL 3.3 Core");
        ImGui::SameLine();
        ImGui::TextDisabled("| ImGui %s | Borderless Custom Title Bar Active", IMGUI_VERSION);
        ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);
        ImGui::Text("FPS: %.1f", io.Framerate);

        ImGui::End();

        // Finalize ImGui and Render to OpenGL
        ImGui::Render();

        int displayW = 0, displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(Palette::Background.x, Palette::Background.y, Palette::Background.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
