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

/**
 * Custom JSON serializer for std::chrono::duration.
 * Converts the value to microseconds.
 */
template <typename Rep, typename Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
    static void to_json(json &j, const std::chrono::duration<Rep, Period> &duration) {
        j = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
};

}

namespace dp {

/**
 * Specifies that a type is an enumeration type that has an element named `Last`.
 */
template<typename Enum>
concept IsEnumWithLast = IsEnum<Enum> && is_valid_value_v<Enum::Last>;

/**
 * Tool for collecting metrics.
 *
 * The collected entries are specified by enumeration types that are given as template parameters.
 * Collected data can be exported into JSON.
 *
 * @tparam CounterEntries Entries with a single accumulated integer value.
 * @tparam SeriesEntries Entries with a vector of integer values.
 * @tparam DurationEntries Entries with a vector of durations.
 * @tparam CumulativeDurationEntries Entries with a single accumulated duration.
 */
template<IsEnumWithLast CounterEntries, IsEnumWithLast SeriesEntries, IsEnumWithLast DurationEntries,
        IsEnumWithLast CumulativeDurationEntries>
class MetricsCollector {
private:
    static constexpr size_t num_counters = to_underlying(CounterEntries::Last) + 1;
    static constexpr size_t num_series = to_underlying(SeriesEntries::Last) + 1;
    static constexpr size_t num_durations = to_underlying(DurationEntries::Last) + 1;
    static constexpr size_t num_cumulative_durations = to_underlying(CumulativeDurationEntries::Last) + 1;

    using counter = int64_t;
    using counter_collection = std::array<counter, num_counters>;

    using series_value = int64_t;
    using series = std::vector<series_value>;
    using series_collection = std::array<series, num_series>;

    using clock = std::chrono::steady_clock;
    using duration = clock::duration;
    using cumulative_duration_collection = std::array<duration, num_cumulative_durations>;

    using durations = std::vector<duration>;
    using durations_collection = std::array<durations, num_durations>;

    template<size_t Size>
    using name_array = std::array<std::string, Size>;

    template<typename Entry>
    class Timer;

public:
    /**
     * Creates a new metrics collector.
     *
     * Names given as constructor arguments correspond to entries supplied as template arguments.
     * They are used in JSON exports.
     */
    MetricsCollector(const name_array<num_counters> &counter_names,
                     const name_array<num_series> &series_names,
                     const name_array<num_durations> &duration_names,
                     const name_array<num_cumulative_durations> &cumulative_duration_names) :
        m_counter_names(counter_names), m_series_names(series_names), m_duration_names(duration_names),
        m_cumulative_duration_names(cumulative_duration_names) {}

    MetricsCollector(const MetricsCollector &) = delete;
    MetricsCollector &operator=(const MetricsCollector &) = delete;
    MetricsCollector(MetricsCollector &&) = delete;
    MetricsCollector &operator=(MetricsCollector &&) = delete;

    /**
     * Adds a given value to a counter.
     */
    void increase_counter(CounterEntries entry, counter amount = 1) {
        m_counters[to_underlying(entry)] += amount;
    }

    /**
     * Appends a given value to a series.
     */
    void append_to_series(SeriesEntries entry, series_value value) {
        m_series[to_underlying(entry)].push_back(value);
    }

    /**
     * Starts a timer who's duration will be appended to a vector entry when stopped.
     *
     * @return Timer that can be stopped manually by .stop(), or will be stopped automatically upon destruction.
     */
    [[nodiscard]] Timer<DurationEntries> get_timer(DurationEntries entry) {
        return Timer<DurationEntries>(*this, entry);
    }

    /**
     * Starts a timer who's duration will be added to a cumulative entry when stopped.
     *
     * @return Timer that can be stopped manually by .stop(), or will be stopped automatically upon destruction.
     */
    [[nodiscard]] Timer<CumulativeDurationEntries> get_cumulative_timer(CumulativeDurationEntries entry) {
        return Timer<CumulativeDurationEntries>(*this, entry);
    }

    /**
     * Exports collected data as JSON to a given stream.
     */
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
        json cumulative_durations;
        for (size_t i = 0; i < num_cumulative_durations; ++i) {
            cumulative_durations[m_cumulative_duration_names[i]] = m_cumulative_durations[i];
        }
        json j;
        j["counters"] = counters;
        j["series"] = series;
        j["durations"] = durations;
        j["cumulative_durations"] = cumulative_durations;
        stream << j.dump(2) << std::endl;
    }

private:
    template<typename Entry>
    class Timer {
    public:
        Timer(MetricsCollector &metrics, Entry entry) :
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
        const Entry m_entry;
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
    cumulative_duration_collection m_cumulative_durations{};
    const name_array<num_counters> &m_counter_names;
    const name_array<num_series> &m_series_names;
    const name_array<num_durations> &m_duration_names;
    const name_array<num_cumulative_durations> &m_cumulative_duration_names;

    void add_duration(DurationEntries entry, duration duration) {
        m_durations[to_underlying(entry)].push_back(duration);
    }

    void add_duration(CumulativeDurationEntries entry, duration duration) {
        m_cumulative_durations[to_underlying(entry)] += duration;
    }
};

} // namespace dp
