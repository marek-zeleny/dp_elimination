#pragma once

#include <string>
#include <optional>

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

private:
    ArgsParser() = default;

    std::string m_input_cnf_file_name;
    std::string m_output_cnf_file_name{"result.cnf"};
    std::string m_log_file_name{"dp.log"};
    size_t m_eliminated_vars{3};
};
