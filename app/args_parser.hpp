#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <tuple>
#include <limits>

/**
 * Encapsulates a CLI argument parser.
 *
 * Uses the CLI11 library.
 */
class ArgsParser {
private:
    using EnumUnderlyingType = uint16_t;

public:
    /**
     * Parses the given input arguments.
     *
     * @return If parsing was successful, returns an instance with the parsed arguments, otherwise returns nullopt.
     */
    static std::optional<ArgsParser> parse(int argc, char *argv[]);

    ArgsParser(const ArgsParser &) = default;
    ArgsParser &operator=(const ArgsParser &) = default;
    ArgsParser(ArgsParser &&) = default;
    ArgsParser &operator=(ArgsParser &&) = default;
    ~ArgsParser() = default;

    enum class Heuristic : EnumUnderlyingType {
        None,
        Ascending,
        Descending,
        MinimalBloat,
    };

    enum class Condition : EnumUnderlyingType {
        None,
        Never,
        Interval,
        RelativeSize,
        AbsoluteSize,
    };

    [[nodiscard]] const std::string &get_config_string() const { return m_config_string; }

    [[nodiscard]] const std::string &get_input_cnf_file_name() const { return m_input_cnf_file_name; }
    [[nodiscard]] const std::string &get_output_cnf_file_name() const { return m_output_cnf_file_name; }
    [[nodiscard]] const std::string &get_log_file_name() const { return m_log_file_name; }
    [[nodiscard]] const std::string &get_metrics_file_name() const { return m_metrics_file_name; }
    [[nodiscard]] const size_t &get_output_cnf_file_max_size() const { return m_output_cnf_file_max_size; }

    [[nodiscard]] Heuristic get_heuristic() const { return m_heuristic; }
    [[nodiscard]] Condition get_complete_minimization_condition() const { return m_complete_minimization_condition; }
    [[nodiscard]] size_t get_complete_minimization_interval() const { return m_complete_minimization_interval; }
    [[nodiscard]] float get_complete_minimization_relative_size() const { return m_complete_minimization_relative_size; }
    [[nodiscard]] Condition get_partial_minimization_condition() const { return m_partial_minimization_condition; }
    [[nodiscard]] size_t get_partial_minimization_interval() const { return m_partial_minimization_interval; }
    [[nodiscard]] float get_partial_minimization_relative_size() const { return m_partial_minimization_relative_size; }
    [[nodiscard]] size_t get_partial_minimization_absolute_size() const { return m_partial_minimization_absolute_size; }
    [[nodiscard]] Condition get_incremental_absorption_removal_condition() const { return m_incremental_absorption_removal_condition; }
    [[nodiscard]] size_t get_incremental_absorption_removal_interval() const { return m_incremental_absorption_removal_interval; }
    [[nodiscard]] float get_incremental_absorption_removal_relative_size() const { return m_incremental_absorption_removal_relative_size; }
    [[nodiscard]] size_t get_incremental_absorption_removal_absolute_size() const { return m_incremental_absorption_removal_absolute_size; }

    [[nodiscard]] const std::optional<size_t> &get_max_iterations() const { return m_max_iterations; }
    [[nodiscard]] const std::optional<size_t> &get_max_duration_seconds() const { return m_max_duration_seconds; }
    [[nodiscard]] const std::optional<float> &get_max_formula_growth() const { return m_max_formula_growth; }
    [[nodiscard]] size_t get_min_var() const { return std::get<0>(m_var_range); }
    [[nodiscard]] size_t get_max_var() const { return std::get<1>(m_var_range); }

    [[nodiscard]] size_t get_sylvan_table_size() const {
        return 1LL << std::get<0>(m_sylvan_table_size_pow);
    }
    [[nodiscard]] size_t get_sylvan_table_max_size() const {
        return 1LL << std::get<1>(m_sylvan_table_size_pow);
    }
    [[nodiscard]] size_t get_sylvan_cache_size() const {
        return 1LL << std::get<0>(m_sylvan_cache_size_pow);
    }
    [[nodiscard]] size_t get_sylvan_cache_max_size() const {
        return 1LL << std::get<1>(m_sylvan_cache_size_pow);
    }
    [[nodiscard]] size_t get_lace_threads() const { return m_lace_threads; }

private:
    ArgsParser() = default;

    std::string m_config_string;
    // files
    std::string m_input_cnf_file_name{};
    std::string m_output_cnf_file_name{"result.cnf"};
    std::string m_log_file_name{"dp.log"};
    std::string m_metrics_file_name{"metrics.json"};
    size_t m_output_cnf_file_max_size{std::numeric_limits<size_t>::max()};
    // algorithm
    Heuristic m_heuristic{Heuristic::None};
    Condition m_complete_minimization_condition{Condition::RelativeSize};
    size_t m_complete_minimization_interval{1};
    float m_complete_minimization_relative_size{1.5};
    Condition m_partial_minimization_condition{Condition::RelativeSize};
    size_t m_partial_minimization_interval{1};
    float m_partial_minimization_relative_size{0.1};
    size_t m_partial_minimization_absolute_size{0};
    Condition m_incremental_absorption_removal_condition{Condition::RelativeSize};
    size_t m_incremental_absorption_removal_interval{1};
    float m_incremental_absorption_removal_relative_size{0.1};
    size_t m_incremental_absorption_removal_absolute_size{0};
    // stop conditions
    std::optional<size_t> m_max_iterations{};
    std::optional<size_t> m_max_duration_seconds{};
    std::optional<float> m_max_formula_growth{};
    std::tuple<size_t, size_t> m_var_range{0, std::numeric_limits<size_t>::max()};
    // sylvan
    std::tuple<uint8_t, uint8_t> m_sylvan_table_size_pow{20, 25};
    std::tuple<uint8_t, uint8_t> m_sylvan_cache_size_pow{20, 25};
    size_t m_lace_threads{1};
};
