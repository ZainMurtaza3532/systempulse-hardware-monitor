#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace SystemPulse::Utils {

    inline std::string FormatBytes(uint64_t bytes) {
        constexpr uint64_t KB = 1024ULL;
        constexpr uint64_t MB = KB * 1024ULL;
        constexpr uint64_t GB = MB * 1024ULL;
        constexpr uint64_t TB = GB * 1024ULL;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);

        if (bytes >= TB) {
            oss << (static_cast<double>(bytes) / TB) << " TB";
        } else if (bytes >= GB) {
            oss << (static_cast<double>(bytes) / GB) << " GB";
        } else if (bytes >= MB) {
            oss << (static_cast<double>(bytes) / MB) << " MB";
        } else if (bytes >= KB) {
            oss << (static_cast<double>(bytes) / KB) << " KB";
        } else {
            oss << bytes << " B";
        }

        return oss.str();
    }

    inline std::string FormatSpeed(uint64_t bytesPerSec) {
        return FormatBytes(bytesPerSec) + "/s";
    }

    inline std::string FormatPercentage(float percent) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << percent << "%";
        return oss.str();
    }

    inline std::string FormatFrequency(float mhz) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        if (mhz >= 1000.0f) {
            oss << (mhz / 1000.0f) << " GHz";
        } else {
            oss << mhz << " MHz";
        }
        return oss.str();
    }

    inline std::string FormatUptime(uint64_t seconds) {
        uint64_t days = seconds / 86400;
        uint64_t hours = (seconds % 86400) / 3600;
        uint64_t minutes = (seconds % 3600) / 60;
        uint64_t secs = seconds % 60;

        std::ostringstream oss;
        if (days > 0) {
            oss << days << "d " << hours << "h " << minutes << "m";
        } else if (hours > 0) {
            oss << hours << "h " << minutes << "m " << secs << "s";
        } else {
            oss << minutes << "m " << secs << "s";
        }
        return oss.str();
    }

} // namespace SystemPulse::Utils

