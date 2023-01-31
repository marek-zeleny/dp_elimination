#pragma once

#include <string>

namespace dp {

class ArgsParser {
public:
    static ArgsParser parse(int argc, char *argv[]);

    bool success() const;
    std::string get_program_name() const;
    std::string get_input_cnf_file_name() const;

private:
    ArgsParser();
    ArgsParser(const std::string &program_name, const std::string &input_cnf_file_name);

    static void print_usage(const std::string &program_name);

    bool m_success;
    std::string m_program_name;
    std::string m_input_cnf_file_name;
};

} // namespace dp
