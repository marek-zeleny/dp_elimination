#pragma once

#include <type_traits>
#include <cstdint>
#include <array>
#include <vector>
#include <chrono>
#include <stdexcept>
#include "utils.hpp"

namespace dp {

template<typename Enum>
concept IsEnumWithLast = IsEnum<Enum> && is_valid_value_v<Enum::Last>;

template<IsEnumWithLast CounterEntries, IsEnumWithLast DurationEntries>
class MetricsCollector {
private:
    class Timer;

public:
    void increase_counter(CounterEntries entry, size_t amount = 1) {
        m_counters[to_underlying(entry)] += amount;
    }

    [[nodiscard]] Timer get_timer(DurationEntries entry) {
        return Timer(*this, entry);
    }

private:
    using counter = size_t;
    using clock = std::chrono::steady_clock;
    using duration = clock::duration;
    using durations = std::vector<duration>;
    using counter_collection = std::array<counter, to_underlying(CounterEntries::Last) + 1>;
    using durations_collection = std::array<durations, to_underlying(DurationEntries::Last) + 1>;

    class Timer {
    public:
        Timer(MetricsCollector &metrics, DurationEntries entry) :
                m_collector(metrics), m_entry(entry), m_start_time(clock::now()) {}

        ~Timer() {
            if (m_running) {
                stop_impl();
            }
        }

        void stop() {
            if (m_running) {
                stop_impl();
            } else {
                throw std::logic_error("Trying to stop a timer that has already stopped");
            }
        }

    private:
        MetricsCollector &m_collector;
        const DurationEntries m_entry;
        const clock::time_point m_start_time;
        bool m_running{true};

        void stop_impl() {
            duration elapsed_time = clock::now() - m_start_time;
            m_collector.add_duration(m_entry, elapsed_time);
            m_running = false;
        }
    };

    counter_collection m_counters{};
    durations_collection m_durations{};

    void add_duration(DurationEntries entry, duration duration) {
        m_durations[to_underlying(entry)].push_back(duration);
    }
};

} // namespace dp
