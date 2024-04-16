#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <tuple>
#include <limits>

class ArgsParser {
private:
    using EnumUnderlyingType = uint16_t;

public:
    static std::optional<ArgsParser> parse(int argc, char *argv[]);

    ArgsParser(const ArgsParser &) = default;
    ArgsParser &operator=(const ArgsParser &) = default;
    ArgsParser(ArgsParser &&) = default;
    ArgsParser &operator=(ArgsParser &&) = default;
    ~ArgsParser() = default;

    enum class Heuristic : EnumUnderlyingType {
        None,
        Simple,
        UnitLiteral,
        ClearLiteral,
        MinimalBloat,
    };

    enum class StopCondition : EnumUnderlyingType {
        None,
        Growth,
        AllVariables,
    };

    enum class AbsorbedRemovalAlgorithm : EnumUnderlyingType {
        None,
        ZBDD,
        WatchedLiterals,
    };

    enum class AbsorbedRemovalCondition : EnumUnderlyingType {
        None,
        Never,
        Interval,
        FormulaGrowth,
    };

    [[nodiscard]] const std::string &get_input_cnf_file_name() const { return m_input_cnf_file_name; }
    [[nodiscard]] const std::string &get_output_cnf_file_name() const { return m_output_cnf_file_name; }
    [[nodiscard]] const std::string &get_log_file_name() const { return m_log_file_name; }
    [[nodiscard]] const std::string &get_metrics_file_name() const { return m_metrics_file_name; }

    [[nodiscard]] Heuristic get_heuristic() const { return m_heuristic; }
    [[nodiscard]] StopCondition get_stop_condition() const { return m_stop_condition; }
    [[nodiscard]] float get_max_formula_growth() const { return m_max_formula_growth; }
    [[nodiscard]] AbsorbedRemovalAlgorithm get_absorbed_removal_algorithm() const { return m_absorbed_removal_algorithm; }
    [[nodiscard]] AbsorbedRemovalCondition get_absorbed_removal_condition() const { return m_absorbed_removal_condition; }
    [[nodiscard]] size_t get_absorbed_removal_interval() const { return m_absorbed_removal_interval; }
    [[nodiscard]] float get_absorbed_removal_growth() const { return m_absorbed_removal_growth; }
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

    std::string m_input_cnf_file_name{};
    std::string m_output_cnf_file_name{"result.cnf"};
    std::string m_log_file_name{"dp.log"};
    std::string m_metrics_file_name{"metrics.json"};

    Heuristic m_heuristic{Heuristic::None};
    StopCondition m_stop_condition{StopCondition::None};
    float m_max_formula_growth{1.0f};
    AbsorbedRemovalAlgorithm m_absorbed_removal_algorithm{AbsorbedRemovalAlgorithm::WatchedLiterals};
    AbsorbedRemovalCondition m_absorbed_removal_condition{AbsorbedRemovalCondition::FormulaGrowth};
    size_t m_absorbed_removal_interval{0};
    float m_absorbed_removal_growth{1.5};
    std::tuple<size_t, size_t> m_var_range{0, std::numeric_limits<size_t>::max()};

    std::tuple<uint8_t, uint8_t> m_sylvan_table_size_pow{20, 25};
    std::tuple<uint8_t, uint8_t> m_sylvan_cache_size_pow{20, 25};
    size_t m_lace_threads{0};
};
