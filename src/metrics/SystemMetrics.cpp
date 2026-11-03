#include "metrics/SystemMetrics.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cmath>

#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iphlpapi.h>
#include <netioapi.h>

#ifdef _MSC_VER
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif

static inline uint64_t FileTimeToUInt64(const FILETIME& ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}
#elif defined(__linux__)
#include <fstream>
#include <string>
#include <vector>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

namespace SystemPulse {

    SystemMetrics::SystemMetrics()
        : m_lastUpdateTime(std::chrono::steady_clock::now()) {
    }

    SystemMetrics::~SystemMetrics() {
#ifdef _WIN32
        ShutdownPdh();
#endif
    }

    SystemMetrics::SystemMetrics(SystemMetrics&& other) noexcept
        : m_snapshot(other.m_snapshot),
          m_lastUpdateTime(other.m_lastUpdateTime),
          m_isInitialized(other.m_isInitialized)
    {
#ifdef _WIN32
        m_prevCpuIdle = other.m_prevCpuIdle;
        m_prevCpuKernel = other.m_prevCpuKernel;
        m_prevCpuUser = other.m_prevCpuUser;
        m_hasCpuBaseline = other.m_hasCpuBaseline;

        m_pdhQuery = other.m_pdhQuery;
        m_pdhDiskReadCounter = other.m_pdhDiskReadCounter;
        m_pdhDiskWriteCounter = other.m_pdhDiskWriteCounter;
        m_pdhInitialized = other.m_pdhInitialized;

        m_prevNetIn = other.m_prevNetIn;
        m_prevNetOut = other.m_prevNetOut;
        m_hasNetBaseline = other.m_hasNetBaseline;

        other.m_pdhQuery = nullptr;
        other.m_pdhDiskReadCounter = nullptr;
        other.m_pdhDiskWriteCounter = nullptr;
        other.m_pdhInitialized = false;
#elif defined(__linux__)
        m_prevCpuTotal = other.m_prevCpuTotal;
        m_prevCpuIdle = other.m_prevCpuIdle;
        m_hasCpuBaseline = other.m_hasCpuBaseline;

        m_prevDiskReadBytes = other.m_prevDiskReadBytes;
        m_prevDiskWriteBytes = other.m_prevDiskWriteBytes;
        m_hasDiskBaseline = other.m_hasDiskBaseline;

        m_prevNetIn = other.m_prevNetIn;
        m_prevNetOut = other.m_prevNetOut;
        m_hasNetBaseline = other.m_hasNetBaseline;
#endif
    }

    SystemMetrics& SystemMetrics::operator=(SystemMetrics&& other) noexcept {
        if (this != &other) {
#ifdef _WIN32
            ShutdownPdh();
#endif
            m_snapshot = other.m_snapshot;
            m_lastUpdateTime = other.m_lastUpdateTime;
            m_isInitialized = other.m_isInitialized;

#ifdef _WIN32
            m_prevCpuIdle = other.m_prevCpuIdle;
            m_prevCpuKernel = other.m_prevCpuKernel;
            m_prevCpuUser = other.m_prevCpuUser;
            m_hasCpuBaseline = other.m_hasCpuBaseline;

            m_pdhQuery = other.m_pdhQuery;
            m_pdhDiskReadCounter = other.m_pdhDiskReadCounter;
            m_pdhDiskWriteCounter = other.m_pdhDiskWriteCounter;
            m_pdhInitialized = other.m_pdhInitialized;

            m_prevNetIn = other.m_prevNetIn;
            m_prevNetOut = other.m_prevNetOut;
            m_hasNetBaseline = other.m_hasNetBaseline;

            other.m_pdhQuery = nullptr;
            other.m_pdhDiskReadCounter = nullptr;
            other.m_pdhDiskWriteCounter = nullptr;
            other.m_pdhInitialized = false;
#elif defined(__linux__)
            m_prevCpuTotal = other.m_prevCpuTotal;
            m_prevCpuIdle = other.m_prevCpuIdle;
            m_hasCpuBaseline = other.m_hasCpuBaseline;

            m_prevDiskReadBytes = other.m_prevDiskReadBytes;
            m_prevDiskWriteBytes = other.m_prevDiskWriteBytes;
            m_hasDiskBaseline = other.m_hasDiskBaseline;

            m_prevNetIn = other.m_prevNetIn;
            m_prevNetOut = other.m_prevNetOut;
            m_hasNetBaseline = other.m_hasNetBaseline;
#endif
        }
        return *this;
    }

    void SystemMetrics::Initialize() {
#ifdef _WIN32
        InitializePdh();
#endif
        m_lastUpdateTime = std::chrono::steady_clock::now();
        m_isInitialized = true;

        // Perform initial baseline measurement
        SampleCpu();
        SampleMemory();
        SampleDisk(1.0);
        SampleNetwork(1.0);
    }

    void SystemMetrics::Update() {
        if (!m_isInitialized) {
            Initialize();
            return;
        }

        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(now - m_lastUpdateTime).count();

        // Enforce a small non-zero threshold to prevent division by zero
        if (elapsedSeconds <= 0.0001) {
            elapsedSeconds = 0.0001;
        }

        SampleCpu();
        SampleMemory();
        SampleDisk(elapsedSeconds);
        SampleNetwork(elapsedSeconds);

        m_lastUpdateTime = now;
        m_snapshot.timestamp = std::chrono::system_clock::now();
    }

    // ---------------------------------------------------------------------------
    // CPU Usage Monitoring
    // ---------------------------------------------------------------------------

    void SystemMetrics::SampleCpu() {
#ifdef _WIN32
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            uint64_t idle = FileTimeToUInt64(idleTime);
            uint64_t kernel = FileTimeToUInt64(kernelTime);
            uint64_t user = FileTimeToUInt64(userTime);

            if (m_hasCpuBaseline) {
                uint64_t diffIdle = idle - m_prevCpuIdle;
                uint64_t diffKernel = kernel - m_prevCpuKernel;
                uint64_t diffUser = user - m_prevCpuUser;

                // On Windows, kernelTime already includes idleTime
                uint64_t diffTotal = diffKernel + diffUser;

                if (diffTotal > 0) {
                    float usage = (1.0f - (static_cast<float>(diffIdle) / static_cast<float>(diffTotal))) * 100.0f;
                    m_snapshot.cpu.usagePercent = std::clamp(usage, 0.0f, 100.0f);
                }
            }

            m_prevCpuIdle = idle;
            m_prevCpuKernel = kernel;
            m_prevCpuUser = user;
            m_hasCpuBaseline = true;
        }
#elif defined(__linux__)
        std::ifstream statFile("/proc/stat");
        if (statFile.is_open()) {
            std::string cpuLabel;
            statFile >> cpuLabel;
            if (cpuLabel == "cpu") {
                uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
                if (statFile >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
                    uint64_t totalIdle = idle + iowait;
                    uint64_t totalNonIdle = user + nice + system + irq + softirq + steal;
                    uint64_t total = totalIdle + totalNonIdle;

                    if (m_hasCpuBaseline && total > m_prevCpuTotal) {
                        uint64_t totalDiff = total - m_prevCpuTotal;
                        uint64_t idleDiff = totalIdle - m_prevCpuIdle;

                        if (totalDiff >= idleDiff && totalDiff > 0) {
                            float usage = (static_cast<float>(totalDiff - idleDiff) / static_cast<float>(totalDiff)) * 100.0f;
                            m_snapshot.cpu.usagePercent = std::clamp(usage, 0.0f, 100.0f);
                        }
                    }

                    m_prevCpuTotal = total;
                    m_prevCpuIdle = totalIdle;
                    m_hasCpuBaseline = true;
                }
            }
        }
#endif
    }

    // ---------------------------------------------------------------------------
    // Memory (RAM) Monitoring
    // ---------------------------------------------------------------------------

    void SystemMetrics::SampleMemory() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);

        if (GlobalMemoryStatusEx(&memInfo)) {
            constexpr double BYTES_TO_GB = 1.0 / (1024.0 * 1024.0 * 1024.0);

            m_snapshot.memory.totalBytes = memInfo.ullTotalPhys;
            m_snapshot.memory.availableBytes = memInfo.ullAvailPhys;
            m_snapshot.memory.usedBytes = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

            m_snapshot.memory.totalGB = static_cast<double>(m_snapshot.memory.totalBytes) * BYTES_TO_GB;
            m_snapshot.memory.availableGB = static_cast<double>(m_snapshot.memory.availableBytes) * BYTES_TO_GB;
            m_snapshot.memory.usedGB = static_cast<double>(m_snapshot.memory.usedBytes) * BYTES_TO_GB;

            m_snapshot.memory.usagePercent = static_cast<float>(memInfo.dwMemoryLoad);
        }
#elif defined(__linux__)
        std::ifstream memFile("/proc/meminfo");
        if (memFile.is_open()) {
            std::string line;
            uint64_t memTotalKB = 0;
            uint64_t memAvailableKB = 0;
            uint64_t memFreeKB = 0;
            uint64_t buffersKB = 0;
            uint64_t cachedKB = 0;
            bool foundAvailable = false;

            while (std::getline(memFile, line)) {
                std::istringstream iss(line);
                std::string key;
                uint64_t val;
                if (iss >> key >> val) {
                    if (key == "MemTotal:") memTotalKB = val;
                    else if (key == "MemAvailable:") { memAvailableKB = val; foundAvailable = true; }
                    else if (key == "MemFree:") memFreeKB = val;
                    else if (key == "Buffers:") buffersKB = val;
                    else if (key == "Cached:") cachedKB = val;
                }
            }

            if (!foundAvailable) {
                memAvailableKB = memFreeKB + buffersKB + cachedKB;
            }

            constexpr double BYTES_TO_GB = 1.0 / (1024.0 * 1024.0 * 1024.0);
            uint64_t totalBytes = memTotalKB * 1024ULL;
            uint64_t availBytes = memAvailableKB * 1024ULL;
            uint64_t usedBytes = (totalBytes >= availBytes) ? (totalBytes - availBytes) : 0;

            m_snapshot.memory.totalBytes = totalBytes;
            m_snapshot.memory.availableBytes = availBytes;
            m_snapshot.memory.usedBytes = usedBytes;

            m_snapshot.memory.totalGB = static_cast<double>(totalBytes) * BYTES_TO_GB;
            m_snapshot.memory.availableGB = static_cast<double>(availBytes) * BYTES_TO_GB;
            m_snapshot.memory.usedGB = static_cast<double>(usedBytes) * BYTES_TO_GB;

            m_snapshot.memory.usagePercent = (totalBytes > 0)
                ? (static_cast<float>(usedBytes) / static_cast<float>(totalBytes) * 100.0f)
                : 0.0f;
        }
#endif
    }

    // ---------------------------------------------------------------------------
    // Disk Activity Monitoring
    // ---------------------------------------------------------------------------

#ifdef _WIN32
    void SystemMetrics::InitializePdh() {
        if (PdhOpenQueryW(nullptr, 0, &m_pdhQuery) == ERROR_SUCCESS) {
            // Using English counter names ensures compatibility on all language editions of Windows
            PDH_STATUS sRead = PdhAddEnglishCounterW(
                m_pdhQuery,
                L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec",
                0,
                &m_pdhDiskReadCounter
            );
            PDH_STATUS sWrite = PdhAddEnglishCounterW(
                m_pdhQuery,
                L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec",
                0,
                &m_pdhDiskWriteCounter
            );

            if (sRead == ERROR_SUCCESS && sWrite == ERROR_SUCCESS) {
                // Collect first sample to establish baseline
                PdhCollectQueryData(m_pdhQuery);
                m_pdhInitialized = true;
            } else {
                // Fallback attempt for LogicalDisk if PhysicalDisk is unavailable
                sRead = PdhAddEnglishCounterW(m_pdhQuery, L"\\LogicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_pdhDiskReadCounter);
                sWrite = PdhAddEnglishCounterW(m_pdhQuery, L"\\LogicalDisk(_Total)\\Disk Write Bytes/sec", 0, &m_pdhDiskWriteCounter);
                if (sRead == ERROR_SUCCESS && sWrite == ERROR_SUCCESS) {
                    PdhCollectQueryData(m_pdhQuery);
                    m_pdhInitialized = true;
                }
            }
        }
    }

    void SystemMetrics::ShutdownPdh() {
        if (m_pdhQuery) {
            PdhCloseQuery(m_pdhQuery);
            m_pdhQuery = nullptr;
            m_pdhDiskReadCounter = nullptr;
            m_pdhDiskWriteCounter = nullptr;
            m_pdhInitialized = false;
        }
    }
#endif

    void SystemMetrics::SampleDisk([[maybe_unused]] double elapsedSeconds) {
#ifdef _WIN32
        if (m_pdhInitialized && m_pdhQuery) {
            PDH_STATUS status = PdhCollectQueryData(m_pdhQuery);
            if (status == ERROR_SUCCESS) {
                PDH_FMT_COUNTERVALUE valRead;
                PDH_FMT_COUNTERVALUE valWrite;

                if (PdhGetFormattedCounterValue(m_pdhDiskReadCounter, PDH_FMT_LARGE, nullptr, &valRead) == ERROR_SUCCESS) {
                    m_snapshot.disk.readBytesPerSec = static_cast<uint64_t>(valRead.largeValue < 0 ? 0 : valRead.largeValue);
                }
                if (PdhGetFormattedCounterValue(m_pdhDiskWriteCounter, PDH_FMT_LARGE, nullptr, &valWrite) == ERROR_SUCCESS) {
                    m_snapshot.disk.writeBytesPerSec = static_cast<uint64_t>(valWrite.largeValue < 0 ? 0 : valWrite.largeValue);
                }

                constexpr double BYTES_TO_MB = 1.0 / (1024.0 * 1024.0);
                m_snapshot.disk.readMBPerSec = static_cast<double>(m_snapshot.disk.readBytesPerSec) * BYTES_TO_MB;
                m_snapshot.disk.writeMBPerSec = static_cast<double>(m_snapshot.disk.writeBytesPerSec) * BYTES_TO_MB;
                m_snapshot.disk.totalBytesRead += static_cast<uint64_t>(m_snapshot.disk.readBytesPerSec * elapsedSeconds);
                m_snapshot.disk.totalBytesWritten += static_cast<uint64_t>(m_snapshot.disk.writeBytesPerSec * elapsedSeconds);
            }
        }
#elif defined(__linux__)
        std::ifstream diskFile("/proc/diskstats");
        if (diskFile.is_open()) {
            std::string line;
            uint64_t totalSectorsRead = 0;
            uint64_t totalSectorsWritten = 0;

            while (std::getline(diskFile, line)) {
                std::istringstream iss(line);
                int major, minor;
                std::string devName;
                uint64_t readsCompleted, readsMerged, sectorsRead, timeReading;
                uint64_t writesCompleted, writesMerged, sectorsWritten, timeWriting;

                if (iss >> major >> minor >> devName
                        >> readsCompleted >> readsMerged >> sectorsRead >> timeReading
                        >> writesCompleted >> writesMerged >> sectorsWritten >> timeWriting) {
                    // Filter out loop devices and RAM disks
                    if (devName.rfind("loop", 0) == 0 || devName.rfind("ram", 0) == 0) continue;

                    // Match root physical drive devices to avoid double-counting partitions
                    bool isPhysicalDisk = (devName.rfind("sd", 0) == 0 && devName.size() == 3) ||
                                         (devName.rfind("vd", 0) == 0 && devName.size() == 3) ||
                                         (devName.rfind("nvme", 0) == 0 && devName.find("p") == std::string::npos);

                    if (isPhysicalDisk) {
                        totalSectorsRead += sectorsRead;
                        totalSectorsWritten += sectorsWritten;
                    }
                }
            }

            // Linux diskstats report sectors of 512 bytes each
            uint64_t currentReadBytes = totalSectorsRead * 512ULL;
            uint64_t currentWrittenBytes = totalSectorsWritten * 512ULL;

            m_snapshot.disk.totalBytesRead = currentReadBytes;
            m_snapshot.disk.totalBytesWritten = currentWrittenBytes;

            if (m_hasDiskBaseline && elapsedSeconds > 0.0) {
                if (currentReadBytes >= m_prevDiskReadBytes) {
                    m_snapshot.disk.readBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(currentReadBytes - m_prevDiskReadBytes) / elapsedSeconds
                    );
                }
                if (currentWrittenBytes >= m_prevDiskWriteBytes) {
                    m_snapshot.disk.writeBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(currentWrittenBytes - m_prevDiskWriteBytes) / elapsedSeconds
                    );
                }

                constexpr double BYTES_TO_MB = 1.0 / (1024.0 * 1024.0);
                m_snapshot.disk.readMBPerSec = static_cast<double>(m_snapshot.disk.readBytesPerSec) * BYTES_TO_MB;
                m_snapshot.disk.writeMBPerSec = static_cast<double>(m_snapshot.disk.writeBytesPerSec) * BYTES_TO_MB;
            }

            m_prevDiskReadBytes = currentReadBytes;
            m_prevDiskWriteBytes = currentWrittenBytes;
            m_hasDiskBaseline = true;
        }
#endif
    }

    // ---------------------------------------------------------------------------
    // Network Bandwidth Monitoring
    // ---------------------------------------------------------------------------

    void SystemMetrics::SampleNetwork(double elapsedSeconds) {
#ifdef _WIN32
        PMIB_IF_TABLE2 pIfTable = nullptr;
        if (GetIfTable2(&pIfTable) == NO_ERROR && pIfTable != nullptr) {
            uint64_t totalIn = 0;
            uint64_t totalOut = 0;

            for (ULONG i = 0; i < pIfTable->NumEntries; ++i) {
                const MIB_IF_ROW2& row = pIfTable->Table[i];
                if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.Type == IF_TYPE_TUNNEL) continue;

                // Check Description and Alias against virtual miniport & filter driver keywords
                char desc[256] = { 0 };
                char alias[256] = { 0 };
                WideCharToMultiByte(CP_UTF8, 0, row.Description, -1, desc, sizeof(desc), nullptr, nullptr);
                WideCharToMultiByte(CP_UTF8, 0, row.Alias, -1, alias, sizeof(alias), nullptr, nullptr);

                std::string descStr = desc;
                std::string aliasStr = alias;

                auto containsCi = [](std::string_view str, std::string_view term) {
                    if (str.size() < term.size()) return false;
                    auto it = std::search(str.begin(), str.end(), term.begin(), term.end(),
                        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
                    return it != str.end();
                };

                if (containsCi(descStr, "WAN Miniport") || containsCi(aliasStr, "WAN Miniport") ||
                    containsCi(descStr, "QoS")          || containsCi(aliasStr, "QoS") ||
                    containsCi(descStr, "WFP")          || containsCi(aliasStr, "WFP") ||
                    containsCi(descStr, "Kernel Debugger") || containsCi(aliasStr, "Kernel Debugger") ||
                    containsCi(descStr, "TAP")          || containsCi(aliasStr, "TAP") ||
                    containsCi(descStr, "Packet Scheduler") || containsCi(descStr, "LightWeight Filter")) {
                    continue;
                }

                // Sum active throughput for connected interfaces
                if (row.OperStatus == IfOperStatusUp) {
                    totalIn += row.InOctets;
                    totalOut += row.OutOctets;
                }
            }
            FreeMibTable(pIfTable);

            m_snapshot.network.totalBytesReceived = totalIn;
            m_snapshot.network.totalBytesSent = totalOut;

            if (m_hasNetBaseline && elapsedSeconds > 0.0) {
                if (totalIn >= m_prevNetIn) {
                    m_snapshot.network.downloadBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalIn - m_prevNetIn) / elapsedSeconds
                    );
                }
                if (totalOut >= m_prevNetOut) {
                    m_snapshot.network.uploadBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalOut - m_prevNetOut) / elapsedSeconds
                    );
                }

                constexpr double BYTES_TO_MB = 1.0 / (1024.0 * 1024.0);
                m_snapshot.network.downloadMBPerSec = static_cast<double>(m_snapshot.network.downloadBytesPerSec) * BYTES_TO_MB;
                m_snapshot.network.uploadMBPerSec = static_cast<double>(m_snapshot.network.uploadBytesPerSec) * BYTES_TO_MB;
            }

            m_prevNetIn = totalIn;
            m_prevNetOut = totalOut;
            m_hasNetBaseline = true;
        }
#elif defined(__linux__)
        std::ifstream netFile("/proc/net/dev");
        if (netFile.is_open()) {
            std::string line;
            // Skip headers
            std::getline(netFile, line);
            std::getline(netFile, line);

            uint64_t totalIn = 0;
            uint64_t totalOut = 0;

            while (std::getline(netFile, line)) {
                auto colonPos = line.find(':');
                if (colonPos == std::string::npos) continue;

                std::string ifName = line.substr(0, colonPos);
                ifName.erase(0, ifName.find_first_not_of(" \t"));
                ifName.erase(ifName.find_last_not_of(" \t") + 1);

                if (ifName == "lo") continue;

                std::istringstream iss(line.substr(colonPos + 1));
                uint64_t rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCompressed, rxMulticast;
                uint64_t txBytes;

                if (iss >> rxBytes >> rxPackets >> rxErrs >> rxDrop >> rxFifo >> rxFrame >> rxCompressed >> rxMulticast >> txBytes) {
                    totalIn += rxBytes;
                    totalOut += txBytes;
                }
            }

            m_snapshot.network.totalBytesReceived = totalIn;
            m_snapshot.network.totalBytesSent = totalOut;

            if (m_hasNetBaseline && elapsedSeconds > 0.0) {
                if (totalIn >= m_prevNetIn) {
                    m_snapshot.network.downloadBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalIn - m_prevNetIn) / elapsedSeconds
                    );
                }
                if (totalOut >= m_prevNetOut) {
                    m_snapshot.network.uploadBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalOut - m_prevNetOut) / elapsedSeconds
                    );
                }

                constexpr double BYTES_TO_MB = 1.0 / (1024.0 * 1024.0);
                m_snapshot.network.downloadMBPerSec = static_cast<double>(m_snapshot.network.downloadBytesPerSec) * BYTES_TO_MB;
                m_snapshot.network.uploadMBPerSec = static_cast<double>(m_snapshot.network.uploadBytesPerSec) * BYTES_TO_MB;
            }

            m_prevNetIn = totalIn;
            m_prevNetOut = totalOut;
            m_hasNetBaseline = true;
        }
#endif
    }

    // ---------------------------------------------------------------------------
    // Formatting Helpers
    // ---------------------------------------------------------------------------

    std::string SystemMetrics::FormatBytes(uint64_t bytes) {
        constexpr uint64_t KB = 1024ULL;
        constexpr uint64_t MB = KB * 1024ULL;
        constexpr uint64_t GB = MB * 1024ULL;
        constexpr uint64_t TB = GB * 1024ULL;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

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

    std::string SystemMetrics::FormatSpeed(uint64_t bytesPerSec) {
        return FormatBytes(bytesPerSec) + "/s";
    }

} // namespace SystemPulse

