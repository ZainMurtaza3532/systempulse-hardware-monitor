#include "ui/Theme.hpp"
#include <algorithm>

namespace SystemPulse::UI::Theme {

    void ApplyDarkTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Base theme configuration
        colors[ImGuiCol_Text]                  = Colors::TextPrimary;
        colors[ImGuiCol_TextDisabled]          = Colors::TextMuted;
        colors[ImGuiCol_WindowBg]              = Colors::Background;
        colors[ImGuiCol_ChildBg]               = Colors::PanelBackground;
        colors[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.12f, 0.16f, 0.98f);
        colors[ImGuiCol_Border]                = Colors::Border;
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = Colors::CardBackground;
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.18f, 0.21f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.22f, 0.26f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBg]               = Colors::Background;
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = Colors::Background;
        colors[ImGuiCol_MenuBarBg]             = Colors::PanelBackground;
        colors[ImGuiCol_ScrollbarBg]           = Colors::Background;
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.32f, 0.42f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = Colors::AccentCyan;
        colors[ImGuiCol_CheckMark]             = Colors::AccentCyan;
        colors[ImGuiCol_SliderGrab]            = Colors::AccentCyan;
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.90f, 1.00f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.16f, 0.19f, 0.26f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.22f, 0.27f, 0.37f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.26f, 0.32f, 0.44f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.22f, 0.30f, 0.80f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.29f, 0.40f, 0.90f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.34f, 0.46f, 1.00f);
        colors[ImGuiCol_Separator]             = Colors::Border;
        colors[ImGuiCol_SeparatorHovered]      = Colors::AccentCyan;
        colors[ImGuiCol_SeparatorActive]       = Colors::AccentCyan;
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.24f, 0.32f, 0.50f);
        colors[ImGuiCol_ResizeGripHovered]     = Colors::AccentCyan;
        colors[ImGuiCol_ResizeGripActive]      = Colors::AccentCyan;
        colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.24f, 0.33f, 1.00f);
        colors[ImGuiCol_TabSelected]           = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
        colors[ImGuiCol_TabDimmed]             = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.14f, 0.17f, 0.23f, 1.00f);
        colors[ImGuiCol_PlotLines]             = Colors::AccentCyan;
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_PlotHistogram]         = Colors::AccentEmerald;
        colors[ImGuiCol_PlotHistogramHovered]  = Colors::AccentCyan;
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.13f, 0.15f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = Colors::Border;
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.18f, 0.20f, 0.27f, 0.60f);

        // Styling geometry & spacing
        style.WindowRounding    = 8.0f;
        style.ChildRounding     = 6.0f;
        style.FrameRounding     = 5.0f;
        style.PopupRounding     = 6.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 6.0f;

        style.WindowPadding     = ImVec2(14.0f, 14.0f);
        style.FramePadding      = ImVec2(10.0f, 6.0f);
        style.ItemSpacing       = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);
        style.ScrollbarSize     = 13.0f;
        style.GrabMinSize       = 10.0f;
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f;
    }

    ImVec4 GetStatusColor(float percentage) {
        if (percentage >= 85.0f) {
            return Colors::DangerRed;
        } else if (percentage >= 65.0f) {
            return Colors::WarningAmber;
        } else {
            return Colors::AccentEmerald;
        }
    }

} // namespace SystemPulse::UI::Theme

