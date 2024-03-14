#pragma once

#include <string>
#include <optional>
#include <tuple>
#include <limits>

class ArgsParser {
public:
    static std::optional<ArgsParser> parse(int argc, char *argv[]);

    ArgsParser(const ArgsParser &) = default;
    ArgsParser &operator=(const ArgsParser &) = default;
    ArgsParser(ArgsParser &&) = default;
    ArgsParser &operator=(ArgsParser &&) = default;
    ~ArgsParser() = default;

    [[nodiscard]] const std::string &get_input_cnf_file_name() const { return m_input_cnf_file_name; }
    [[nodiscard]] const std::string &get_output_cnf_file_name() const { return m_output_cnf_file_name; }
    [[nodiscard]] const std::string &get_log_file_name() const { return m_log_file_name; }
    [[nodiscard]] size_t get_eliminated_vars() const { return m_eliminated_vars; }
    [[nodiscard]] size_t get_absorbed_clause_elimination_interval() const { return m_absorbed_clause_elimination_interval; }
    [[nodiscard]] size_t get_min_var() const { return std::get<0>(m_var_range); }
    [[nodiscard]] size_t get_max_var() const { return std::get<1>(m_var_range); }
    [[nodiscard]] size_t get_sylvan_table_size() const { return std::get<0>(m_sylvan_table_size); }
    [[nodiscard]] size_t get_sylvan_table_max_size() const { return std::get<1>(m_sylvan_table_size); }
    [[nodiscard]] size_t get_sylvan_cache_size() const { return std::get<0>(m_sylvan_cache_size); }
    [[nodiscard]] size_t get_sylvan_cache_max_size() const { return std::get<1>(m_sylvan_cache_size); }

private:
    ArgsParser() = default;

    std::string m_input_cnf_file_name;
    std::string m_output_cnf_file_name{"result.cnf"};
    std::string m_log_file_name{"dp.log"};
    size_t m_eliminated_vars{3};
    size_t m_absorbed_clause_elimination_interval{0};
    std::tuple<size_t, size_t> m_var_range{0, std::numeric_limits<size_t>::max()};
    std::tuple<size_t, size_t> m_sylvan_table_size{1LL<<22, 1LL<<26};
    std::tuple<size_t, size_t> m_sylvan_cache_size{1LL<<22, 1LL<<26};
};
