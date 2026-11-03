#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace SystemPulse::Core {

    struct WindowConfig {
        std::string title{"SystemPulse - Modern Hardware Monitor"};
        int width{1280};
        int height{800};
        bool vsync{true};
    };

    class Window {
    public:
        explicit Window(const WindowConfig& config = WindowConfig{});
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept;
        Window& operator=(Window&&) noexcept;

        [[nodiscard]] bool ShouldClose() const;
        void PollEvents() const;
        void SwapBuffers() const;

        [[nodiscard]] GLFWwindow* GetNativeHandle() const { return m_window; }
        [[nodiscard]] int GetWidth() const { return m_width; }
        [[nodiscard]] int GetHeight() const { return m_height; }

    private:
        GLFWwindow* m_window{nullptr};
        int m_width{1280};
        int m_height{800};

        void InitializeGlfw(const WindowConfig& config);
        void ApplyImmersiveDarkMode();
    };

} // namespace SystemPulse::Core

