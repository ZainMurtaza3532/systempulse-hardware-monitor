#pragma once

#include "monitors/ISystemMonitor.hpp"
#include "ui/RingBuffer.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace SystemPulse::Monitors {

    struct NetworkAdapter {
        std::string name;          // Friendly name, e.g. "Wi-Fi" or "Ethernet"
        std::string description;   // Hardware description, e.g. "Intel(R) Wi-Fi 6 AX201"
        std::string macAddress;    // MAC address e.g. "00:1A:2B:3C:4D:5E"
        std::string ipAddress;     // Primary IPv4 address e.g. "192.168.1.15"
        std::string type;          // "Wi-Fi", "Ethernet", or "vEthernet"
        bool isOperational{false};
        uint64_t bytesReceived{0};
        uint64_t bytesSent{0};
        uint64_t linkSpeedBps{0};  // Speed in bits per second (e.g. 1 Gbps = 1000000000)
    };

    struct NetworkTelemetry {
        std::vector<NetworkAdapter> adapters;
        uint64_t currentDownloadSpeed{0}; // Bytes / sec
        uint64_t currentUploadSpeed{0};   // Bytes / sec
        uint64_t totalBytesReceived{0};
        uint64_t totalBytesSent{0};

        SystemPulse::UI::RingBuffer<float, 60> downloadHistory;
        SystemPulse::UI::RingBuffer<float, 60> uploadHistory;
    };

    class NetworkMonitor : public ISystemMonitor {
    public:
        NetworkMonitor();
        ~NetworkMonitor() override = default;

        void Initialize() override;
        void Update() override;
        [[nodiscard]] std::string_view GetName() const override { return "Network Monitor"; }

        [[nodiscard]] const NetworkTelemetry& GetTelemetry() const { return m_telemetry; }

    private:
        NetworkTelemetry m_telemetry;
        uint64_t m_lastInOctets{0};
        uint64_t m_lastOutOctets{0};
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        bool m_hasInitialReading{false};

        void QueryNetwork();
    };

} // namespace SystemPulse::Monitors
