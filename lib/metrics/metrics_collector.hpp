#pragma once

#include <type_traits>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <ostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "utils.hpp"

namespace nlohmann {

template <typename Rep, typename Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
    static void to_json(json &j, const std::chrono::duration<Rep, Period> &duration) {
        j = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
};

}

namespace dp {

template<typename Enum>
concept IsEnumWithLast = IsEnum<Enum> && is_valid_value_v<Enum::Last>;

template<IsEnumWithLast CounterEntries, IsEnumWithLast SeriesEntries, IsEnumWithLast DurationEntries>
class MetricsCollector {
private:
    static constexpr size_t num_counters = to_underlying(CounterEntries::Last) + 1;
    static constexpr size_t num_series = to_underlying(SeriesEntries::Last) + 1;
    static constexpr size_t num_durations = to_underlying(DurationEntries::Last) + 1;

    using counter = size_t;
    using counter_collection = std::array<counter, num_counters>;

    using series = std::vector<counter>;
    using series_collection = std::array<series, num_series>;

    using clock = std::chrono::steady_clock;
    using duration = clock::duration;
    using durations = std::vector<duration>;
    using durations_collection = std::array<durations, num_durations>;

    template<size_t Size>
    using name_array = std::array<std::string, Size>;

    class Timer;

public:
    MetricsCollector(const name_array<num_counters> &counter_names,
                     const name_array<num_series> &series_names,
                     const name_array<num_durations> &duration_names) :
        m_counter_names(counter_names), m_series_names(series_names), m_duration_names(duration_names) {}

    MetricsCollector(const MetricsCollector &) = delete;
    MetricsCollector &operator=(const MetricsCollector &) = delete;
    MetricsCollector(MetricsCollector &&) = delete;
    MetricsCollector &operator=(MetricsCollector &&) = delete;

    void increase_counter(CounterEntries entry, size_t amount = 1) {
        m_counters[to_underlying(entry)] += amount;
    }

    void append_to_series(SeriesEntries entry, counter value) {
        m_series[to_underlying(entry)].push_back(value);
    }

    [[nodiscard]] Timer get_timer(DurationEntries entry) {
        return Timer(*this, entry);
    }

    void export_json(std::ostream &stream) {
        using json = nlohmann::json;
        json counters;
        for (size_t i = 0; i < num_counters; ++i) {
            counters[m_counter_names[i]] = m_counters[i];
        }
        json series;
        for (size_t i = 0; i < num_series; ++i) {
            series[m_series_names[i]] = m_series[i];
        }
        json durations;
        for (size_t i = 0; i < num_durations; ++i) {
            durations[m_duration_names[i]] = m_durations[i];
        }
        json j;
        j["counters"] = counters;
        j["series"] = series;
        j["durations"] = durations;
        stream << j.dump(2) << std::endl;
    }

private:
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
    series_collection m_series{};
    durations_collection m_durations{};
    const name_array<num_counters> &m_counter_names;
    const name_array<num_series> &m_series_names;
    const name_array<num_durations> &m_duration_names;

    void add_duration(DurationEntries entry, duration duration) {
        m_durations[to_underlying(entry)].push_back(duration);
    }
};

} // namespace dp
