#pragma once

#include <vector>
#include <tuple>
#include <unordered_set>

namespace dp {

class WatchedLiterals {
public:
    using Literal = int32_t;
    using Clause = std::vector<Literal>;

    enum class Assignment : int8_t {unassigned = 0, negative = -1, positive = 1};

    WatchedLiterals(const std::vector<Clause> &clauses, size_t min_var, size_t max_var);
    WatchedLiterals(const std::vector<Clause> &clauses, size_t min_var, size_t max_var,
        const std::unordered_set<size_t> &deactivated_clauses);

    static WatchedLiterals from_vector(const std::vector<Clause> &clauses);
    static WatchedLiterals from_vector(const std::vector<Clause> &clauses,
        const std::unordered_set<size_t> &deactivated_clauses);
    static Assignment negate(const Assignment &a);

    bool contains_empty() const;
    size_t get_assignment_level() const;
    bool assign_value(Literal l);
    Assignment get_assignment(Literal l) const;
    void backtrack(size_t num_levels);
    void backtrack_to(size_t target_level);
    void change_active_clauses(const std::vector<size_t> &activate_indices,
        const std::vector<size_t> &deactivate_indices);
    void print_clauses() const;
    void print_stack() const;

private:
    using StackElement = std::vector<Literal>;

    struct ClauseData {
        const Clause &clause;
        size_t watched1 {0};
        size_t watched2 {0};
        bool is_active {false};

        ClauseData(const Clause &c) : clause(c) {}
    };

    struct VarData {
        std::unordered_set<size_t> watched_clauses {};
        Assignment assignment {Assignment::unassigned};
    };

    const size_t m_min_var;
    const size_t m_max_var;
    std::vector<ClauseData> m_clauses;
    std::vector<VarData> m_variables;
    std::vector<StackElement> m_stack;
    std::unordered_set<size_t> m_unit_clauses;
    std::unordered_set<size_t> m_initial_unit_clauses;
    size_t m_empty_count {0};
    size_t m_initial_empty_count {0};

    void activate_clause(size_t cluase_index, bool skip_if_active);
    void deactivate_clause(size_t clause_index, bool skip_if_not_active);
    bool propagate();
    bool assign_value_impl(Literal l);
    bool update_watched_literal(size_t clause_index, size_t var_index);
    void backtrack_impl();

    size_t get_var_index(Literal l) const;
};

} // namespace dp
