#pragma once

#include <string_view>

namespace SystemPulse::Monitors {

    class ISystemMonitor {
    public:
        virtual ~ISystemMonitor() = default;

        virtual void Initialize() = 0;
        virtual void Update() = 0;
        [[nodiscard]] virtual std::string_view GetName() const = 0;
    };

} // namespace SystemPulse::Monitors

