#pragma once

#include <imgui.h>

namespace SystemPulse::UI::Theme {

    // Modern cyberpunk / slate color palette
    namespace Colors {
        constexpr ImVec4 Background       = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
        constexpr ImVec4 PanelBackground  = ImVec4(0.11f, 0.13f, 0.18f, 1.00f);
        constexpr ImVec4 CardBackground   = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
        constexpr ImVec4 Border           = ImVec4(0.22f, 0.25f, 0.34f, 0.60f);

        // Accent indicators
        constexpr ImVec4 AccentCyan       = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
        constexpr ImVec4 AccentEmerald    = ImVec4(0.00f, 0.90f, 0.46f, 1.00f);
        constexpr ImVec4 AccentViolet     = ImVec4(0.55f, 0.35f, 1.00f, 1.00f);
        constexpr ImVec4 WarningAmber     = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
        constexpr ImVec4 DangerRed        = ImVec4(1.00f, 0.30f, 0.30f, 1.00f);

        // Typography
        constexpr ImVec4 TextPrimary      = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        constexpr ImVec4 TextSecondary    = ImVec4(0.60f, 0.64f, 0.72f, 1.00f);
        constexpr ImVec4 TextMuted        = ImVec4(0.40f, 0.44f, 0.52f, 1.00f);
    }

    void ApplyDarkTheme();

    // Returns a dynamic color shifting from green -> yellow -> red based on percentage (0-100)
    ImVec4 GetStatusColor(float percentage);

} // namespace SystemPulse::UI::Theme

