#include "core/Window.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

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
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

namespace SystemPulse::Core {

    static void GlfwErrorCallback(int error, const char* description) {
        std::cerr << "[GLFW Error " << error << "]: " << description << std::endl;
    }

    Window::Window(const WindowConfig& config)
        : m_width(config.width), m_height(config.height) {
        InitializeGlfw(config);
    }

    Window::~Window() {
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }

    Window::Window(Window&& other) noexcept
        : m_window(other.m_window), m_width(other.m_width), m_height(other.m_height) {
        other.m_window = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept {
        if (this != &other) {
            if (m_window) {
                glfwDestroyWindow(m_window);
            }
            m_window = other.m_window;
            m_width = other.m_width;
            m_height = other.m_height;
            other.m_window = nullptr;
        }
        return *this;
    }

    void Window::InitializeGlfw(const WindowConfig& config) {
        glfwSetErrorCallback(GlfwErrorCallback);

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW library.");
        }

        // OpenGL 3.3 Core profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        m_window = glfwCreateWindow(m_width, m_height, config.title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW application window.");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(config.vsync ? 1 : 0);

        // Resize framebuffer callback
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
            auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
            if (self) {
                self->m_width = w;
                self->m_height = h;
            }
        });

        ApplyImmersiveDarkMode();
    }

    void Window::ApplyImmersiveDarkMode() {
#ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(m_window);
        if (hwnd) {
            BOOL useDarkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
        }
#endif
    }

    bool Window::ShouldClose() const {
        return m_window ? glfwWindowShouldClose(m_window) : true;
    }

    void Window::PollEvents() const {
        glfwPollEvents();
    }

    void Window::SwapBuffers() const {
        if (m_window) {
            glfwSwapBuffers(m_window);
        }
    }

} // namespace SystemPulse::Core

