#include "io/cnf_reader.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace dp {

void dp::CnfReader::read_from_file(const std::string &file_name, AddClauseFunction &func) {
    std::ifstream file(file_name);
    std::string line;
    bool started = false;
    Clause curr_clause;
    size_t clause_count = 0;
    size_t num_vars;
    size_t num_clauses;
    size_t line_num = 0;
    while (std::getline(file, line)) {
        ++line_num;
        if (line.empty()) {
            continue;
        } else if (line.length() == 1 && line[0] == '\r') {
            // when compiling on Linux and reading a Windows file, this deals with CRLF line endings
            continue;
        } else if (line[0] == 'c') {
            continue;
        }
        if (!started) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            if (token == "p") {
                std::string type;
                if ((iss >> type >> num_vars >> num_clauses) && type == "cnf") { // TODO
                    started = true;
                    continue;
                } else {
                    throw failure("invalid problem definition (p)", line_num);
                }
            } else {
                throw failure("doesn't contain problem definition (p).", line_num);
            }
        }
        std::istringstream iss(line);
        Literal literal;
        while ((iss >> literal)) {
            if (literal == 0) {
                func(curr_clause);
                curr_clause.clear();
                ++clause_count;
            } else {
                curr_clause.push_back(literal);
            }
        }
    }
    // the final 0 might be omitted
    if (curr_clause.size() > 0) {
        func(curr_clause);
        ++clause_count;
    }
    if (clause_count != num_clauses) {
        throw warning("the number of clauses doesn't correspond to the problem definition (p)", line_num);
    }
}

dp::CnfReader::failure::failure(const std::string &what_arg, const size_t line_num)
    : std::runtime_error(construct_msg(what_arg, line_num)) {}

std::string CnfReader::failure::construct_msg(const std::string &msg, const size_t line_num) {
    std::ostringstream oss;
    oss << "Invalid CNF input file [line " << line_num << "]: " << msg;
    return oss.str();
}

CnfReader::warning::warning(const std::string &what_arg, const size_t line_num)
    : std::runtime_error(construct_msg(what_arg, line_num)) {}

std::string CnfReader::warning::construct_msg(const std::string &msg, const size_t line_num) {
    std::ostringstream oss;
    oss << "CNF input file format warning [line " << line_num << "]: " << msg;
    return oss.str();
}

} // namespace dp
