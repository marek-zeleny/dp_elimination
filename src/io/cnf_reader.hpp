#pragma once

#include <vector>
#include <string>
#include <functional>
#include <stdexcept>

namespace dp {

class CnfReader {
public:
    class failure : public std::runtime_error {
    public:
        failure(const std::string &what_arg, const size_t line_num);

    private:
        static std::string construct_msg(const std::string &msg, const size_t line_num);
    };

    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using AddClauseFunction = std::function<void(Clause &clause)>;

    static void read_from_file(const std::string &file_name, AddClauseFunction &func);
    static std::vector<Clause> read_from_file_to_vector(const std::string &file_name);

private:
    static void print_warning(const std::string &msg, const size_t line_num);
};

} // namespace dp
