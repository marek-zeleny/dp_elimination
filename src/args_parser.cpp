#include "args_parser.hpp"

#include <iostream>

namespace dp {

ArgsParser::ArgsParser() : m_success(false) {}

ArgsParser::ArgsParser(const std::string &program_name, const std::string &input_cnf_file_name) :
    m_success(true),
    m_program_name(program_name),
    m_input_cnf_file_name(input_cnf_file_name)
    {}

ArgsParser ArgsParser::parse(int argc, char *argv[]) {
    std::string program_name {argv[0]};
    if (argc != 2) {
        print_usage(program_name);
        return ArgsParser();
    }

    std::string input_cnf_file_name {argv[1]};
    return ArgsParser(program_name, input_cnf_file_name);
}

bool ArgsParser::success() const {
    return m_success;
}

std::string ArgsParser::get_program_name() const {
    return m_program_name;
}

std::string ArgsParser::get_input_cnf_file_name() const {
    return m_input_cnf_file_name;
}

void ArgsParser::print_usage(const std::string &program_name) {
    std::cerr << "Usage:" << std::endl << std::endl;
    std::cerr << program_name;
    // args:
    std::cerr << " <INPUT CNF FILE>";

    std::cerr << std::endl;
}

} // namespace dp
