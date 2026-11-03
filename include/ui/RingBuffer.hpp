#pragma once

#include <vector>
#include <array>
#include <numeric>
#include <algorithm>
#include <cstddef>

namespace SystemPulse::UI {

    template <typename T, size_t Capacity>
    class RingBuffer {
    public:
        RingBuffer() : m_head(0), m_count(0) {
            m_buffer.fill(static_cast<T>(0));
        }

        void Push(T value) {
            m_buffer[m_head] = value;
            m_head = (m_head + 1) % Capacity;
            if (m_count < Capacity) {
                m_count++;
            }
        }

        [[nodiscard]] size_t Size() const { return m_count; }
        [[nodiscard]] constexpr size_t MaxCapacity() const { return Capacity; }

        [[nodiscard]] T Latest() const {
            if (m_count == 0) return static_cast<T>(0);
            size_t idx = (m_head + Capacity - 1) % Capacity;
            return m_buffer[idx];
        }

        // Fills an output buffer ordered chronologically (oldest to newest)
        // suitable for direct plotting in ImGui::PlotLines
        void CopyLinear(std::array<T, Capacity>& outArray) const {
            if (m_count == 0) {
                outArray.fill(static_cast<T>(0));
                return;
            }

            size_t start = (m_count < Capacity) ? 0 : m_head;
            for (size_t i = 0; i < Capacity; ++i) {
                if (i < m_count) {
                    size_t readIdx = (start + i) % Capacity;
                    outArray[i] = m_buffer[readIdx];
                } else {
                    outArray[i] = static_cast<T>(0);
                }
            }
        }

        [[nodiscard]] T Average() const {
            if (m_count == 0) return static_cast<T>(0);
            T sum = std::accumulate(m_buffer.begin(), m_buffer.begin() + m_count, static_cast<T>(0));
            return sum / static_cast<T>(m_count);
        }

        [[nodiscard]] T Max() const {
            if (m_count == 0) return static_cast<T>(0);
            auto it = std::max_element(m_buffer.begin(), m_buffer.begin() + m_count);
            return *it;
        }

        [[nodiscard]] T Min() const {
            if (m_count == 0) return static_cast<T>(0);
            auto it = std::min_element(m_buffer.begin(), m_buffer.begin() + m_count);
            return *it;
        }

        void Clear() {
            m_buffer.fill(static_cast<T>(0));
            m_head = 0;
            m_count = 0;
        }

    private:
        std::array<T, Capacity> m_buffer;
        size_t m_head;
        size_t m_count;
    };

} // namespace SystemPulse::UI

