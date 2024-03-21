#pragma once

#include <type_traits>
#include <cstdint>
#include <array>
#include <vector>
#include <chrono>
#include "utils.hpp"

namespace dp {

template<typename Enum>
concept IsEnumWithLast = IsEnum<Enum> && is_valid_value_v<Enum::Last>;

template<IsEnumWithLast CounterEntries, IsEnumWithLast DurationEntries>
class MetricsCollector {
private:
    using counter = size_t;
    using clock = std::chrono::steady_clock;
    using duration = clock::duration;
    using durations = std::vector<duration>;
    using counter_collection = std::array<counter, to_underlying(CounterEntries::Last) + 1>;
    using durations_collection = std::array<durations, to_underlying(DurationEntries::Last) + 1>;

public:
    class Timer {
        Timer(MetricsCollector &metrics, DurationEntries entry) :
            m_collector(metrics), m_entry(entry), m_start_time(clock::now()) {}

        ~Timer() {
            duration elapsed_time = clock::now() - m_start_time;
            m_collector.add_duration(m_entry, elapsed_time);
        }

    private:
        MetricsCollector &m_collector;
        const DurationEntries m_entry;
        const clock::time_point m_start_time;
    };

    void counter_inc(CounterEntries entry) {
        ++m_counters[to_underlying(entry)];
    }

    [[nodiscard]] Timer get_timer(DurationEntries entry) {
        return Timer(*this, entry);
    }

private:
    counter_collection m_counters{};
    durations_collection m_durations{};

    void add_duration(DurationEntries entry, duration duration) {
        m_durations[to_underlying(entry)].push_back(duration);
    }
};

} // namespace dp
