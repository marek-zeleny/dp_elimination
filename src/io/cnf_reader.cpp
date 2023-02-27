#include "io/cnf_reader.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <limits>

namespace dp {

void CnfReader::read_from_file(const std::string &file_name, AddClauseFunction &func) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
        throw failure("failed to open the input file", 0);
    }
    std::string line;
    bool started = false;
    Clause curr_clause;
    size_t clause_count = 0;
    size_t num_vars;
    size_t num_clauses;
    size_t min_var = std::numeric_limits<size_t>::max();
    size_t max_var = std::numeric_limits<size_t>::min();
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
                size_t var = (size_t)std::abs(literal);
                if (var > max_var) {
                    if (var - min_var > num_vars) {
                        print_warning("variable outside the range defind in the problem definition (p)", line_num);
                    } else {
                        max_var = var;
                    }
                }
                if (var < min_var) {
                    if (max_var - var > num_vars) {
                        print_warning("variable outside the range defind in the problem definition (p)", line_num);
                    } else {
                        min_var = var;
                    }
                }
            }
        }
    }
    // the final 0 might be omitted
    if (curr_clause.size() > 0) {
        func(curr_clause);
        ++clause_count;
    }
    if (clause_count != num_clauses) {
        print_warning("the number of clauses doesn't match the problem definition (p)", line_num);
    }
}

std::vector<CnfReader::Clause> CnfReader::read_from_file_to_vector(const std::string &file_name) {
    std::vector<Clause> clauses;
    AddClauseFunction func = [&](const Clause &c) {
        clauses.push_back(c);
    };
    try {
        read_from_file(file_name, func);
    } catch (const failure &f) {
        std::cerr << f.what() << std::endl;
        return std::vector<Clause>();
    }
    return clauses;
}

void CnfReader::print_warning(const std::string &msg, const size_t line_num) {
    std::cerr << "CNF input file format warning [line " << line_num << "]: " << msg << std::endl;
}

CnfReader::failure::failure(const std::string &what_arg, const size_t line_num)
    : std::runtime_error(construct_msg(what_arg, line_num)) {}

std::string CnfReader::failure::construct_msg(const std::string &msg, const size_t line_num) {
    std::ostringstream oss;
    oss << "Invalid CNF input file [line " << line_num << "]: " << msg;
    return oss.str();
}

} // namespace dp
