#include "monitors/MemoryMonitor.hpp"

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
#include <windows.h>
#endif

namespace SystemPulse::Monitors {

    MemoryMonitor::MemoryMonitor() = default;

    void MemoryMonitor::Initialize() {
        Update();
    }

    void MemoryMonitor::Update() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);

        if (GlobalMemoryStatusEx(&memInfo)) {
            m_telemetry.totalPhysicalBytes = memInfo.ullTotalPhys;
            m_telemetry.availablePhysicalBytes = memInfo.ullAvailPhys;
            m_telemetry.usedPhysicalBytes = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
            m_telemetry.usagePercentage = static_cast<float>(memInfo.dwMemoryLoad);

            m_telemetry.totalPageFileBytes = memInfo.ullTotalPageFile;
            m_telemetry.usedPageFileBytes = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
        }
#else
        // Mock fallback for non-Windows platforms
        m_telemetry.totalPhysicalBytes = 32ULL * 1024ULL * 1024ULL * 1024ULL;
        m_telemetry.usedPhysicalBytes = 14ULL * 1024ULL * 1024ULL * 1024ULL;
        m_telemetry.availablePhysicalBytes = m_telemetry.totalPhysicalBytes - m_telemetry.usedPhysicalBytes;
        m_telemetry.usagePercentage = 43.8f;
        m_telemetry.totalPageFileBytes = 48ULL * 1024ULL * 1024ULL * 1024ULL;
        m_telemetry.usedPageFileBytes = 18ULL * 1024ULL * 1024ULL * 1024ULL;
#endif

        m_telemetry.usageHistory.Push(m_telemetry.usagePercentage);
    }

} // namespace SystemPulse::Monitors

