#pragma once

#include <vector>
#include <string>
#include <functional>

namespace dp {

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
    [[nodiscard]] VectorCnf &subset0(Literal l);
    [[nodiscard]] VectorCnf &subset1(Literal l);
    [[nodiscard]] VectorCnf &unify(const VectorCnf &other);
    [[nodiscard]] VectorCnf &intersect(const VectorCnf &other);
    [[nodiscard]] VectorCnf &subtract(const VectorCnf &other);
    [[nodiscard]] VectorCnf &multiply(const VectorCnf &other);
    [[nodiscard]] VectorCnf &remove_tautologies();
    [[nodiscard]] VectorCnf &remove_subsumed_clauses();
    void for_all_clauses(ClauseFunction &func) const;

    [[nodiscard]] std::vector<Clause> to_vector() const;

private:
    std::vector<Clause> m_clauses;

    explicit VectorCnf(std::vector<Clause> &&clauses);
};

} // namespace dp
