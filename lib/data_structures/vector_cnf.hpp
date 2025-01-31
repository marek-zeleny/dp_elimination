#pragma once

#include <vector>
#include <string>
#include <functional>
#include <iostream>

namespace dp {

/**
 * CNF formula represented by a vector of clauses.
 *
 * This implementation was used only for debugging and verification purposes and is not up-to-date.
 */
class VectorCnf {
public:
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using ClauseFunction = std::function<bool(const Clause &)>;

    VectorCnf();

    static VectorCnf from_vector(const std::vector<Clause> &clauses);
    static VectorCnf from_file(const std::string &file_name);

    inline bool operator==(const VectorCnf &other) const {
        return m_clauses == other.m_clauses;
    }

    [[nodiscard]] size_t count_clauses() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] bool contains_empty() const;
    VectorCnf &subset0(Literal l);
    VectorCnf &subset1(Literal l);
    VectorCnf &unify(const VectorCnf &other);
    VectorCnf &intersect(const VectorCnf &other);
    VectorCnf &subtract(const VectorCnf &other);
    VectorCnf &multiply(const VectorCnf &other);
    VectorCnf &remove_tautologies();
    VectorCnf &remove_subsumed_clauses();
    void for_all_clauses(ClauseFunction &func) const;

    [[nodiscard]] std::vector<Clause> to_vector() const;

    void print_clauses(std::ostream &output = std::cout) const;

private:
    std::vector<Clause> m_clauses;

    explicit VectorCnf(std::vector<Clause> &&clauses);
};

} // namespace dp
