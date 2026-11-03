#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>
#endif

namespace SystemPulse {

    // ---------------------------------------------------------------------------
    // Telemetry Metric Structures
    // ---------------------------------------------------------------------------

    struct CpuMetrics {
        float usagePercent{0.0f}; // 0.0% to 100.0%
    };

    struct MemoryMetrics {
        uint64_t totalBytes{0};
        uint64_t availableBytes{0};
        uint64_t usedBytes{0};

        double totalGB{0.0};
        double availableGB{0.0};
        double usedGB{0.0};

        float usagePercent{0.0f}; // 0.0% to 100.0%
    };

    struct DiskMetrics {
        uint64_t readBytesPerSec{0};
        uint64_t writeBytesPerSec{0};

        double readMBPerSec{0.0};
        double writeMBPerSec{0.0};

        uint64_t totalBytesRead{0};
        uint64_t totalBytesWritten{0};
    };

    struct NetworkMetrics {
        uint64_t downloadBytesPerSec{0};
        uint64_t uploadBytesPerSec{0};

        double downloadMBPerSec{0.0};
        double uploadMBPerSec{0.0};

        uint64_t totalBytesReceived{0};
        uint64_t totalBytesSent{0};
    };

    struct SystemMetricsSnapshot {
        CpuMetrics cpu;
        MemoryMetrics memory;
        DiskMetrics disk;
        NetworkMetrics network;
        std::chrono::system_clock::time_point timestamp;
    };

    // ---------------------------------------------------------------------------
    // Cross-Platform System Metrics Collector
    // ---------------------------------------------------------------------------

    class SystemMetrics {
    public:
        SystemMetrics();
        ~SystemMetrics();

        // Non-copyable, movable
        SystemMetrics(const SystemMetrics&) = delete;
        SystemMetrics& operator=(const SystemMetrics&) = delete;
        SystemMetrics(SystemMetrics&&) noexcept;
        SystemMetrics& operator=(SystemMetrics&&) noexcept;

        // Lifecycle methods
        void Initialize();
        void Update();

        // Telemetry accessors
        [[nodiscard]] const SystemMetricsSnapshot& GetSnapshot() const noexcept { return m_snapshot; }
        [[nodiscard]] const CpuMetrics& GetCpu() const noexcept { return m_snapshot.cpu; }
        [[nodiscard]] const MemoryMetrics& GetMemory() const noexcept { return m_snapshot.memory; }
        [[nodiscard]] const DiskMetrics& GetDisk() const noexcept { return m_snapshot.disk; }
        [[nodiscard]] const NetworkMetrics& GetNetwork() const noexcept { return m_snapshot.network; }

        // Formatting utilities
        [[nodiscard]] static std::string FormatBytes(uint64_t bytes);
        [[nodiscard]] static std::string FormatSpeed(uint64_t bytesPerSec);

    private:
        SystemMetricsSnapshot m_snapshot;
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        bool m_isInitialized{false};

        // --- Platform-specific sampling helpers ---
        void SampleCpu();
        void SampleMemory();
        void SampleDisk(double elapsedSeconds);
        void SampleNetwork(double elapsedSeconds);

        // --- Platform-specific state ---
#ifdef _WIN32
        // CPU baseline (GetSystemTimes)
        uint64_t m_prevCpuIdle{0};
        uint64_t m_prevCpuKernel{0};
        uint64_t m_prevCpuUser{0};
        bool m_hasCpuBaseline{false};

        // Disk activity (Windows PDH)
        PDH_HQUERY m_pdhQuery{nullptr};
        PDH_HCOUNTER m_pdhDiskReadCounter{nullptr};
        PDH_HCOUNTER m_pdhDiskWriteCounter{nullptr};
        bool m_pdhInitialized{false};

        // Network baseline (GetIfTable2)
        uint64_t m_prevNetIn{0};
        uint64_t m_prevNetOut{0};
        bool m_hasNetBaseline{false};

        void InitializePdh();
        void ShutdownPdh();
#elif defined(__linux__)
        // CPU baseline (/proc/stat)
        uint64_t m_prevCpuTotal{0};
        uint64_t m_prevCpuIdle{0};
        bool m_hasCpuBaseline{false};

        // Disk baseline (/proc/diskstats)
        uint64_t m_prevDiskReadBytes{0};
        uint64_t m_prevDiskWriteBytes{0};
        bool m_hasDiskBaseline{false};

        // Network baseline (/proc/net/dev)
        uint64_t m_prevNetIn{0};
        uint64_t m_prevNetOut{0};
        bool m_hasNetBaseline{false};
#endif
    };

} // namespace SystemPulse

