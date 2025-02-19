#include "io/cnf_reader.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <tuple>
#include <stdexcept>
#include <limits>
#include <simple_logger.h>

namespace dp {

static bool skip_line(const std::string &line) {
    // last condition: when compiling on Linux and reading a Windows file, this deals with CRLF line endings
    return line.empty() || line[0] == 'c' || (line.length() == 1 && line[0] == '\r');
}

static std::tuple<size_t, size_t> try_start_reading(const std::string &line, const size_t &line_num) {
    size_t num_vars;
    size_t num_clauses;
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    if (token == "p") {
        std::string type;
        if ((iss >> type >> num_vars >> num_clauses) && type == "cnf") {
            LOG_INFO << "reading CNF formula with " << num_vars << " variables and " << num_clauses << " clauses";
            return std::make_tuple(num_vars, num_clauses);
        } else {
            throw CnfReader::failure("invalid problem definition (p)", line_num);
        }
    } else {
        throw CnfReader::failure("doesn't contain problem definition (p).", line_num);
    }
}

void CnfReader::read_from_stream(std::istream &input, AddClauseFunction &func) {
    std::string line;
    bool started = false;
    Clause curr_clause;
    size_t clause_count = 0;
    size_t num_vars;
    size_t num_clauses;
    size_t percent_size;
    size_t min_var = std::numeric_limits<size_t>::max();
    size_t max_var = std::numeric_limits<size_t>::min();
    size_t line_num = 0;
    while (std::getline(input, line)) {
        ++line_num;
        if (skip_line(line)) {
            continue;
        }
        if (!started) {
            std::tie(num_vars, num_clauses) = try_start_reading(line, line_num);
            started = true;
            percent_size = num_clauses / 100;
            if (percent_size == 0) {
                percent_size = 1;
            }
        }
        std::istringstream iss(line);
        Literal literal;
        while ((iss >> literal)) {
            if (literal == 0) {
                func(curr_clause);
                curr_clause.clear();
                ++clause_count;
                if (clause_count % percent_size == 0) {
                    LOG_DEBUG << "Reading progress: " << clause_count << "/" << num_clauses
                              << " (" << 100 * clause_count / num_clauses << " %)";
                }
            } else {
                curr_clause.push_back(literal);
                auto var = static_cast<size_t>(std::abs(literal));
                if (var > max_var) {
                    if (var - min_var > num_vars) {
                        print_warning("variable outside the range defined in the problem definition (p)", line_num);
                    } else {
                        max_var = var;
                    }
                }
                if (var < min_var) {
                    if (max_var - var > num_vars) {
                        print_warning("variable outside the range defined in the problem definition (p)", line_num);
                    } else {
                        min_var = var;
                    }
                }
            }
        }
    }
    // the final 0 might be omitted
    if (!curr_clause.empty()) {
        func(curr_clause);
        ++clause_count;
    }

    if (clause_count != num_clauses) {
        print_warning("the number of clauses doesn't match the problem definition (p)", line_num);
    }
    LOG_INFO << "CNF formula successfully read";
}

void CnfReader::read_from_file(const std::string &file_name, AddClauseFunction &func) {
    LOG_DEBUG << "opening file " << file_name;
    std::ifstream file(file_name);
    if (!file.is_open()) {
        throw failure("failed to open the input file " + file_name, 0);
    }
    read_from_stream(file, func);
}

std::vector<CnfReader::Clause> CnfReader::read_from_file_to_vector(const std::string &file_name) {
    std::vector<Clause> clauses;
    AddClauseFunction func = [&](const Clause &c) {
        clauses.push_back(c);
    };
    read_from_file(file_name, func);
    return clauses;
}

void CnfReader::print_warning(const std::string &msg, const size_t line_num) {
    std::ostringstream oss;
    oss << "CNF input file format warning [line " << line_num << "]: " << msg;
    std::string message = oss.str();
    LOG_WARNING << message;
    std::cerr << message << std::endl;
}

CnfReader::failure::failure(const std::string &what_arg, const size_t line_num) :
        std::runtime_error(construct_msg(what_arg, line_num)) {}

std::string CnfReader::failure::construct_msg(const std::string &msg, const size_t line_num) {
    std::ostringstream oss;
    oss << "Invalid CNF input file [line " << line_num << "]: " << msg;
    return oss.str();
}

} // namespace dp
