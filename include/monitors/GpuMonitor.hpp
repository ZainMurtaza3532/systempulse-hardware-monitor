#pragma once

#include "monitors/ISystemMonitor.hpp"
#include "ui/RingBuffer.hpp"
#include <string>
#include <cstdint>

namespace SystemPulse::Monitors {

    struct GpuTelemetry {
        std::string name{"Discrete / Integrated GPU"};
        std::string driverVersion{"N/A"};
        std::string pcieBusId{"PCIe x16"};
        std::string apiBackend{"DirectX DXGI"};
        std::string directXVersion{"DirectX 12"};

        uint64_t dedicatedMemoryBytes{0};
        uint64_t sharedMemoryBytes{0};
        uint64_t usedMemoryBytes{0};

        float coreUsagePercentage{0.0f};       // 3D Utilization (0.0% - 100.0%)
        float memoryControllerUsage{0.0f};     // Memory controller load (0.0% - 100.0%)
        float temperatureCelsius{0.0f};        // Core temperature in °C
        uint32_t fanSpeedPercentage{0};        // Fan speed in %
        float powerUsageWatts{0.0f};           // Power draw in Watts

        bool isNvmlActive{false};

        SystemPulse::UI::RingBuffer<float, 60> usageHistory;
        SystemPulse::UI::RingBuffer<float, 60> vramHistory;
    };

    class GpuMonitor : public ISystemMonitor {
    public:
        GpuMonitor();
        ~GpuMonitor() override;

        void Initialize() override;
        void Update() override;
        [[nodiscard]] std::string_view GetName() const override { return "GPU Monitor"; }

        [[nodiscard]] const GpuTelemetry& GetTelemetry() const { return m_telemetry; }

    private:
        GpuTelemetry m_telemetry;

        // Dynamic NVML loader state (loaded via LoadLibraryA/GetProcAddress)
        void* m_nvmlModule{nullptr};
        void* m_nvmlDeviceHandle{nullptr};
        bool m_nvmlInitialized{false};

        bool InitializeNvml();
        void ShutdownNvml();
        bool QueryNvmlTelemetry();

        void QueryDxgiAdapter();
    };

} // namespace SystemPulse::Monitors
