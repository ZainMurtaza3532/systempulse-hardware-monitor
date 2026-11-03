#pragma once

#include "monitors/ISystemMonitor.hpp"
#include "ui/RingBuffer.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace SystemPulse::Monitors {

    struct CpuTelemetry {
        std::string modelName{"Unknown Processor"};
        uint32_t physicalCores{0};
        uint32_t logicalCores{0};
        float overallUsage{0.0f};          // 0.0 - 100.0%
        float clockSpeedMhz{0.0f};
        uint32_t activeProcesses{0};
        uint32_t activeThreads{0};
        std::vector<float> perCoreUsage;
        SystemPulse::UI::RingBuffer<float, 60> usageHistory;
    };

    class CpuMonitor : public ISystemMonitor {
    public:
        CpuMonitor();
        ~CpuMonitor() override = default;

        void Initialize() override;
        void Update() override;
        [[nodiscard]] std::string_view GetName() const override { return "CPU Monitor"; }

        [[nodiscard]] const CpuTelemetry& GetTelemetry() const { return m_telemetry; }

    private:
        CpuTelemetry m_telemetry;

#ifdef _WIN32
        uint64_t m_prevIdleTime{0};
        uint64_t m_prevKernelTime{0};
        uint64_t m_prevUserTime{0};
#endif
        void QueryHardwareInfo();
        void CalculateCpuUsage();
    };

} // namespace SystemPulse::Monitors

