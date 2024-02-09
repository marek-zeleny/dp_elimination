#pragma once

#include <string>

class ArgsParser {
public:
    static ArgsParser parse(int argc, char *argv[]);

    [[nodiscard]] bool success() const;
    [[nodiscard]] std::string get_program_name() const;
    [[nodiscard]] std::string get_input_cnf_file_name() const;
    [[nodiscard]] size_t get_eliminated_vars() const;

private:
    ArgsParser();
    ArgsParser(std::string program_name, std::string input_cnf_file_name, size_t eliminated_vars);

    static void print_usage(const std::string &program_name);

    bool m_success;
    std::string m_program_name;
    std::string m_input_cnf_file_name;
    size_t m_eliminated_vars {};
};
