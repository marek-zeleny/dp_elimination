#include "args_parser.hpp"

#include <iostream>
#include <utility>

namespace dp {

ArgsParser::ArgsParser() : m_success(false) {}

ArgsParser::ArgsParser(std::string program_name, std::string input_cnf_file_name, size_t eliminated_vars) :
        m_success(true),
        m_program_name(std::move(program_name)),
        m_input_cnf_file_name(std::move(input_cnf_file_name)),
        m_eliminated_vars(eliminated_vars) {}

ArgsParser ArgsParser::parse(int argc, char *argv[]) {
    std::string program_name{argv[0]};
    if (argc < 2) {
        print_usage(program_name);
        return {};
    }

    std::string input_cnf_file_name{argv[1]};
    size_t eliminated_vars = 3;
    if (argc >= 3) {
        std::string s{argv[2]};
        eliminated_vars = std::stoll(s);
    }
    return {program_name, input_cnf_file_name, eliminated_vars};
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

size_t ArgsParser::get_eliminated_vars() const {
    return m_eliminated_vars;
}

void ArgsParser::print_usage(const std::string &program_name) {
    std::cerr << "Usage:" << std::endl << std::endl;
    std::cerr << program_name;
    // args:
    std::cerr << " <INPUT CNF FILE>";
    std::cerr << " [ELIMINATED VARS COUNT (3)]";

    std::cerr << std::endl;
}

} // namespace dp
