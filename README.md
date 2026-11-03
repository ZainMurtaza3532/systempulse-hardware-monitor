# SystemPulse — Modern Real-Time System Hardware & Performance Monitor

A high-performance desktop system monitoring application built with **C++20**, **Dear ImGui (Docking Branch)**, and **GLFW / OpenGL 3.3**.

---

## Features

- **Automated Dependency Management**: Uses modern CMake `FetchContent` to download and link **GLFW 3.4** and **Dear ImGui (docking branch)** on demand with zero manual installation required.
- **Modern Hardware Telemetry**:
  - **CPU Monitoring**: Overall processor utilization, per-core workload grid, model identification, frequency, active processes and threads.
  - **Memory (RAM) & Paging**: Live physical memory consumption, available headroom, and pagefile metrics.
  - **GPU Engine & VRAM**: Dedicated NVIDIA NVML dynamic loading (`nvml.dll` via `LoadLibraryA`) with automatic DXGI fallback, 3D core load, GPU temperature (°C), live VRAM usage (used vs total GB), fan speed (%), power wattage (W), PCIe bus location, and driver version.
  - **Drive & Storage Activity**: Volume labels, drive capacity, free space indicators, and real-time read/write I/O throughput.
  - **Network Adapter Throughput**: Network interface enumeration, live download and upload speed meters, and packet octets counter.
- **Cyberpunk Dark UI Aesthetic**:
  - Custom dark theme with obsidian slate backgrounds and neon cyan/emerald/amber/red accent indicators.
  - Rounded corners, sleek progress indicators, and 60-second rolling sparkline history charts.
  - Native Windows 10/11 immersive dark mode window frame.
  - ImGui multi-viewport & docking enabled.

---

## Project Structure

```text
Modern System Hardware & Network Monitor/
├── CMakeLists.txt              # Root CMake build definition (FetchContent for GLFW & ImGui)
├── CMakePresets.json           # Standardized configure and build presets
├── .gitignore                  # Git ignore rules for build artifacts and IDE cache
├── README.md                   # Project documentation and build guide
├── assets/                     # Fonts, icons, and graphical assets
│   └── README.md
├── include/
│   ├── SystemMetrics.hpp       # Convenience root header for cross-platform metrics
│   ├── metrics/
│   │   └── SystemMetrics.hpp   # Core cross-platform (Win/Linux) hardware telemetry collector
│   ├── core/
│   │   ├── Application.hpp     # Application lifecycle and render loop
│   │   └── Window.hpp          # GLFW window, context, and dark titlebar handling
│   ├── monitors/
│   │   ├── ISystemMonitor.hpp  # Abstract monitor interface
│   │   ├── CpuMonitor.hpp      # CPU utilization and frequency provider
│   │   ├── MemoryMonitor.hpp   # RAM and page file provider
│   │   ├── GpuMonitor.hpp      # GPU and VRAM provider
│   │   ├── DiskMonitor.hpp     # Storage volumes and I/O rate provider
│   │   └── NetworkMonitor.hpp  # Adapter and bandwidth provider
│   ├── ui/
│   │   ├── Theme.hpp           # Color palette, rounding, and styling
│   │   ├── DashboardView.hpp   # Main dashboard layout, cards, and tabs
│   │   └── RingBuffer.hpp      # High-performance fixed-size telemetry history buffer
│   └── utils/
│       └── Formatters.hpp      # Byte, speed, frequency, and percentage formatting
└── src/
    ├── main.cpp                # Application entry point
    ├── metrics/
    │   └── SystemMetrics.cpp   # Win32 (GetSystemTimes, GlobalMemoryStatusEx, PDH, GetIfTable2) & Linux (/proc)
    ├── core/
    │   ├── Application.cpp     # Loop timing, ImGui context, and rendering
    │   └── Window.cpp          # Window creation and Win32 immersive dark mode
    ├── monitors/
    │   ├── CpuMonitor.cpp      # GetSystemTimes, CPUID, and per-core telemetry
    │   ├── MemoryMonitor.cpp   # GlobalMemoryStatusEx provider
    │   ├── GpuMonitor.cpp      # DXGI adapter enumeration and telemetry
    │   ├── DiskMonitor.cpp     # GetLogicalDriveStringsW and GetDiskFreeSpaceExW
    │   └── NetworkMonitor.cpp  # GetIfTable2 network interface counters
    └── ui/
        ├── Theme.cpp           # ImGui style implementation
        └── DashboardView.cpp   # Dashboard views, KPI cards, and rolling charts
```

---

## Prerequisites

- **C++20 Compliant Compiler**:
  - Microsoft Visual C++ (MSVC) 2019 version 16.10+ or 2022 (Recommended on Windows)
  - GCC 11+ or Clang 13+
- **CMake**: Version **3.20** or higher
- **Git**: Installed and accessible from your `PATH` (used by `FetchContent` to download GLFW and Dear ImGui)

---

## Build Instructions

### Method 1: Command Line (CMake)

1. Open a terminal (PowerShell, Command Prompt, or Bash) in the project directory:
   ```powershell
   # Configure the project (FetchContent will automatically download GLFW and ImGui)
   cmake -B build -DCMAKE_BUILD_TYPE=Release

   # Build the executable
   cmake --build build --config Release
   ```

2. Run the application:
   ```powershell
   ./build/bin/Release/SystemPulse.exe
   # or on Ninja:
   ./build/bin/SystemPulse.exe
   ```

### Method 2: CMake Presets (Visual Studio 2022 or VS Code)

If using Visual Studio 2022:
1. Open the project folder (`File` -> `Open` -> `Folder...`).
2. Select the **`Windows x64 Release (MSVC)`** or **`Windows x64 Debug (MSVC)`** preset from the target configuration dropdown.
3. Visual Studio will automatically configure CMake and download dependencies.
4. Press **F5** to build and run.

If using VS Code with CMake Tools extension:
1. Open the folder in VS Code.
2. Select preset `windows-msvc-release` or `ninja-release`.
3. Press **F7** to build or **F5** to debug.

---

## Roadmap

- [x] **Step 1**: Initial setup with CMake, FetchContent (GLFW 3.4 & Dear ImGui docking), modular architecture, dark theme, and telemetry dashboard skeleton.
- [ ] **Step 2**: Advanced CPU Metrics (Per-core affinity, thermal sensor integration via WMI / WinRing0 / LibreHardwareMonitor).
- [ ] **Step 3**: Dedicated GPU telemetry via NVAPI (NVIDIA) and ADL (AMD) for exact temperatures, fan curves, and power wattage.
- [ ] **Step 4**: Disk Physical I/O queue length and process-level disk usage.
- [ ] **Step 5**: Process Manager tab (Sortable process table by CPU, RAM, Disk, and Network with Kill Process capability).
- [ ] **Step 6**: System tray minimization, user alert thresholds (Audio/Visual notification on high CPU/RAM load), and persistent settings.

