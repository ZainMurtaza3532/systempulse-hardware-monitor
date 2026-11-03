#pragma once

#include "monitors/ISystemMonitor.hpp"
#include "ui/RingBuffer.hpp"
#include <cstdint>

namespace SystemPulse::Monitors {

    struct MemoryTelemetry {
        uint64_t totalPhysicalBytes{0};
        uint64_t usedPhysicalBytes{0};
        uint64_t availablePhysicalBytes{0};
        float usagePercentage{0.0f};

        uint64_t totalPageFileBytes{0};
        uint64_t usedPageFileBytes{0};

        SystemPulse::UI::RingBuffer<float, 60> usageHistory;
    };

    class MemoryMonitor : public ISystemMonitor {
    public:
        MemoryMonitor();
        ~MemoryMonitor() override = default;

        void Initialize() override;
        void Update() override;
        [[nodiscard]] std::string_view GetName() const override { return "Memory Monitor"; }

        [[nodiscard]] const MemoryTelemetry& GetTelemetry() const { return m_telemetry; }

    private:
        MemoryTelemetry m_telemetry;
    };

} // namespace SystemPulse::Monitors

