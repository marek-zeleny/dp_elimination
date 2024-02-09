#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

namespace dp {

class CnfWriter {
public:
    class failure : public std::runtime_error {
    public:
        explicit failure(const std::string &what_arg);

    private:
        static std::string construct_msg(const std::string &msg);
    };

    using Literal = int32_t;
    using Clause = std::vector<Literal>;

    static void write_vector_to_file(const std::vector<Clause> &clauses, const std::string &file_name);

    CnfWriter(std::ostream &output, size_t max_var, size_t num_classes);
    CnfWriter(const std::string &file_name, size_t max_var, size_t num_clauses);

    CnfWriter &write_clause(const Clause &clause);
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
