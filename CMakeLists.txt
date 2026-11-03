cmake_minimum_required(VERSION 3.20)
project(SystemPulse
    VERSION 1.0.0
    DESCRIPTION "Modern Real-Time System Hardware & Performance Monitor"
    LANGUAGES CXX C
)

# ---------------------------------------------------------------------------
# C++ Standard Configuration
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# System & Graphics Dependencies
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# Dependency Management via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Don't check git updates for existing dependencies" FORCE)

# 1. GLFW (v3.4)
message(STATUS "Fetching GLFW 3.4...")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
    UPDATE_COMMAND ""
)
FetchContent_MakeAvailable(glfw)

# 2. Dear ImGui (Docking branch for modern multi-window and docking support)
message(STATUS "Fetching Dear ImGui (docking branch)...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
    UPDATE_COMMAND ""
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    # ImGui does not provide an official CMakeLists.txt, so we create a library target
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui PUBLIC
        glfw
        OpenGL::GL
    )

    # Dear ImGui uses its internal lightweight OpenGL loader (imgui_impl_opengl3_loader.h) by default
endif()

# ---------------------------------------------------------------------------
# Application Sources and Headers
# ---------------------------------------------------------------------------
set(PROJECT_HEADERS
    include/SystemMetrics.hpp
    include/metrics/SystemMetrics.hpp
    include/core/Application.hpp
    include/core/Window.hpp
    include/monitors/ISystemMonitor.hpp
    include/monitors/CpuMonitor.hpp
    include/monitors/MemoryMonitor.hpp
    include/monitors/GpuMonitor.hpp
    include/monitors/DiskMonitor.hpp
    include/monitors/NetworkMonitor.hpp
    include/ui/Theme.hpp
    include/ui/DashboardView.hpp
    include/ui/RingBuffer.hpp
    include/utils/Formatters.hpp
    include/utils/EnvLoader.hpp
)

set(PROJECT_SOURCES
    src/main.cpp
    src/utils/EnvLoader.cpp
    src/metrics/SystemMetrics.cpp
    src/core/Application.cpp
    src/core/Window.cpp
    src/monitors/CpuMonitor.cpp
    src/monitors/MemoryMonitor.cpp
    src/monitors/GpuMonitor.cpp
    src/monitors/DiskMonitor.cpp
    src/monitors/NetworkMonitor.cpp
    src/ui/Theme.cpp
    src/ui/DashboardView.cpp
)

add_executable(${PROJECT_NAME}
    ${PROJECT_HEADERS}
    ${PROJECT_SOURCES}
)

# ---------------------------------------------------------------------------
# Include Directories & Compilation Flags
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glfw
    OpenGL::GL
)

# Compiler-specific warnings and flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /utf-8
        /Zc:__cplusplus
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Windows-specific system libraries for modern UI and hardware telemetry
# Note: NVIDIA NVML (nvml.dll) is dynamically loaded via LoadLibraryA at runtime,
# ensuring zero SDK installation requirement and full portability across MinGW/UCRT64 & MSVC.
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dwmapi    # Windows Desktop Window Manager (immersive dark mode)
        pdh       # Performance Data Helper (CPU counters)
        iphlpapi  # IP Helper API (Network adapter throughput)
        dxgi      # DirectX Graphics Infrastructure (GPU details fallback)
        psapi     # Process Status API
        ws2_32    # Windows Sockets API
    )
endif()

# ---------------------------------------------------------------------------
# Install / Assets Target
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    install(DIRECTORY assets/ DESTINATION bin/assets)
endif()
