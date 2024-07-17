#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

namespace dp {

/**
 * Class for exporting CNF formulas in the DIMACS CNF format.
 */
class CnfWriter {
public:
    /**
     * Error while writing a CNF formula.
     */
    class failure : public std::runtime_error {
    public:
        explicit failure(const std::string &what_arg);

    private:
        static std::string construct_msg(const std::string &msg);
    };

    /**
     * Represents a literal of a propositional variable.
     * Positive literals are positive numbers and vice versa.
     * 0 is an invalid literal.
     */
    using Literal = int32_t;
    using Clause = std::vector<Literal>;

    /**
     * Writes a formula given as a vector of clauses to a given file.
     */
    static void write_vector_to_file(const std::vector<Clause> &clauses, const std::string &file_name);

    /**
     * Creates a writer into a given stream.
     *
     * Upon instantiation writes a header into the stream.
     *
     * @param max_var Maximum variable allowed in clauses to be written.
     * @param num_classes Number of clauses to be written.
     */
    CnfWriter(std::ostream &output, size_t max_var, size_t num_classes);
    /**
     * Creates a writer into a given file.
     *
     * Upon instantiation writes a header into the file.
     *
     * @param max_var Maximum variable allowed in clauses to be written.
     * @param num_classes Number of clauses to be written.
     */
    CnfWriter(const std::string &file_name, size_t max_var, size_t num_clauses);

    /**
     * Writes next clause into the open stream.
     */
    CnfWriter &write_clause(const Clause &clause);
    /**
     * Performs final checks, flushes and closes the stream.
     */
    void finish();

private:
    using Var = uint32_t;

    std::ofstream m_file{};
    std::ostream &m_output;
    const size_t m_max_var;
    const size_t m_num_clauses;
    size_t m_clause_count = 0;
    bool finished = false;

    void write_header();
};

} // namespace dp
