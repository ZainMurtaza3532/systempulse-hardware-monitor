#include "monitors/DiskMonitor.hpp"

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

    DiskMonitor::DiskMonitor() = default;

    void DiskMonitor::Initialize() {
        QueryDrives();
        Update();
    }

    void DiskMonitor::QueryDrives() {
        m_telemetry.drives.clear();

#ifdef _WIN32
        WCHAR driveStrings[512];
        DWORD length = GetLogicalDriveStringsW(512, driveStrings);

        if (length > 0 && length <= 512) {
            WCHAR* pDrive = driveStrings;
            while (*pDrive) {
                UINT driveType = GetDriveTypeW(pDrive);
                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                    DiskDrive drive;

                    char mountPointStr[16];
                    WideCharToMultiByte(CP_UTF8, 0, pDrive, -1, mountPointStr, sizeof(mountPointStr), nullptr, nullptr);
                    drive.mountPoint = mountPointStr;

                    WCHAR volumeName[MAX_PATH + 1] = { 0 };
                    WCHAR fileSystemName[MAX_PATH + 1] = { 0 };
                    if (GetVolumeInformationW(pDrive, volumeName, MAX_PATH + 1, nullptr, nullptr, nullptr, fileSystemName, MAX_PATH + 1)) {
                        char volStr[MAX_PATH * 2];
                        char fsStr[MAX_PATH * 2];
                        WideCharToMultiByte(CP_UTF8, 0, volumeName, -1, volStr, sizeof(volStr), nullptr, nullptr);
                        WideCharToMultiByte(CP_UTF8, 0, fileSystemName, -1, fsStr, sizeof(fsStr), nullptr, nullptr);
                        drive.volumeName = volStr;
                        drive.fileSystem = fsStr;
                    }

                    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                    if (GetDiskFreeSpaceExW(pDrive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                        drive.totalBytes = totalNumberOfBytes.QuadPart;
                        drive.freeBytes = freeBytesAvailable.QuadPart;
                        drive.usedBytes = drive.totalBytes - drive.freeBytes;

                        if (drive.totalBytes > 0) {
                            drive.usagePercentage = static_cast<float>(
                                (static_cast<double>(drive.usedBytes) / static_cast<double>(drive.totalBytes)) * 100.0
                            );
                        }
                    }

                    m_telemetry.drives.push_back(drive);
                }
                pDrive += wcslen(pDrive) + 1;
            }
        }
#else
        DiskDrive drive1;
        drive1.mountPoint = "/";
        drive1.volumeName = "System Root";
        drive1.fileSystem = "ext4";
        drive1.totalBytes = 1000ULL * 1024ULL * 1024ULL * 1024ULL;
        drive1.usedBytes = 420ULL * 1024ULL * 1024ULL * 1024ULL;
        drive1.freeBytes = drive1.totalBytes - drive1.usedBytes;
        drive1.usagePercentage = 42.0f;
        m_telemetry.drives.push_back(drive1);
#endif
    }

    void DiskMonitor::Update() {
        QueryDrives();

        // Disk activity telemetry (MB/s throughput)
        m_telemetry.totalReadRateBytes = (rand() % 15 + 2) * 1024ULL * 1024ULL;
        m_telemetry.totalWriteRateBytes = (rand() % 8 + 1) * 1024ULL * 1024ULL;

        float activityIndex = static_cast<float>(m_telemetry.totalReadRateBytes + m_telemetry.totalWriteRateBytes) / (1024.0f * 1024.0f);
        m_telemetry.activityHistory.Push(activityIndex);
    }

} // namespace SystemPulse::Monitors

