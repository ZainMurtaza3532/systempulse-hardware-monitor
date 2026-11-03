#pragma once

#include "monitors/ISystemMonitor.hpp"
#include "ui/RingBuffer.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace SystemPulse::Monitors {

    struct DiskDrive {
        std::string mountPoint;
        std::string volumeName;
        std::string fileSystem;
        uint64_t totalBytes{0};
        uint64_t freeBytes{0};
        uint64_t usedBytes{0};
        float usagePercentage{0.0f};
    };

    struct DiskTelemetry {
        std::vector<DiskDrive> drives;
        uint64_t totalReadRateBytes{0};
        uint64_t totalWriteRateBytes{0};

        SystemPulse::UI::RingBuffer<float, 60> activityHistory;
    };

    class DiskMonitor : public ISystemMonitor {
    public:
        DiskMonitor();
        ~DiskMonitor() override = default;

        void Initialize() override;
        void Update() override;
        [[nodiscard]] std::string_view GetName() const override { return "Disk Monitor"; }

        [[nodiscard]] const DiskTelemetry& GetTelemetry() const { return m_telemetry; }

    private:
        DiskTelemetry m_telemetry;
        void QueryDrives();
    };

} // namespace SystemPulse::Monitors

