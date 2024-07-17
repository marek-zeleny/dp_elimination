#pragma once

#include <vector>
#include <string>
#include <istream>
#include <functional>
#include <stdexcept>

namespace dp {

/**
 * Static class encapsulating functions for reading CNF formula in the DIMACS CNF format.
 */
class CnfReader {
public:
    /**
     * Error while reading a CNF formula.
     */
    class failure : public std::runtime_error {
    public:
        failure(const std::string &what_arg, size_t line_num);

    private:
        static std::string construct_msg(const std::string &msg, size_t line_num);
    };

    /**
     * Represents a literal of a propositional variable.
     * Positive literals are positive numbers and vice versa.
     * 0 is an invalid literal.
     */
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using AddClauseFunction = std::function<void(Clause &clause)>;

    /**
     * Reads a CNF formula from a given stream and calls given callback function for each parsed clause.
     */
    static void read_from_stream(std::istream &stream, AddClauseFunction &func);
    /**
     * Reads a CNF formula from a given file and calls given callback function for each parsed clause.
     */
    static void read_from_file(const std::string &file_name, AddClauseFunction &func);
    /**
     * Reads a CNF formula from a given file returns it as a vector of clauses.
     */
    static std::vector<Clause> read_from_file_to_vector(const std::string &file_name);

private:
    static void print_warning(const std::string &msg, size_t line_num);
};

} // namespace dp
