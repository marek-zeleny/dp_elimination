#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace dp {

class CnfWriter {
public:
    class failure : public std::runtime_error {
    public:
        failure(const std::string &what_arg);

    private:
        static std::string construct_msg(const std::string &msg);
    };

    using Literal = int32_t;
    using Clause = std::vector<Literal>;

    static bool write_vector_to_file(const std::vector<Clause> &clauses, const std::string &file_name);

    CnfWriter(const std::string &file_name, const size_t max_var, const size_t num_clauses);

    CnfWriter &write_clause(const Clause &clause);
    void finish();

private:
    using Var = uint32_t;

    std::ofstream m_file;
    const size_t m_max_var;
    const size_t m_num_clauses;
    size_t m_clause_count = 0;
    bool finished = false;
};

} // namespace dp
