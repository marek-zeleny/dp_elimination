#pragma once

#include <vector>
#include <tuple>
#include <unordered_set>
#include <iostream>

namespace dp {

/**
 * Watched literals data structure for unit propagation over CNF formulas.
 *
 * The data structure works on top of an existing set (or multiple sets) of clauses, it does not copy the formula.
 * Make sure that the underlying data are valid as long as this instance is used.
 */
class WatchedLiterals {
public:
    /**
     * Represents a literal of a propositional variable.
     * Positive literals are positive numbers and vice versa.
     * 0 is an invalid literal.
     */
    using Literal = int32_t;
    using Clause = std::vector<Literal>;

    enum class Assignment : int8_t {
        unassigned = 0, negative = -1, positive = 1
    };

    /**
     * Creates an instance on top of a vector of clauses.
     *
     * @param max_var Maximum variable appearing in the set of clauses.
     */
    WatchedLiterals(const std::vector<Clause> &clauses, size_t max_var);
    /**
     * Creates an instance on top of a vector of clauses, with some of the clauses deactivated by default.
     * @param max_var Maximum variable appearing in the set of clauses.
     */
    WatchedLiterals(const std::vector<Clause> &clauses, size_t max_var,
                    const std::unordered_set<size_t> &deactivated_clauses);

    /**
     * Creates an instance on top of a vector of clauses.
     */
    static WatchedLiterals from_vector(const std::vector<Clause> &clauses);
    /**
     * Creates an instance on top of a vector of clauses, with some of the clauses deactivated by default.
     */
    static WatchedLiterals from_vector(const std::vector<Clause> &clauses,
                                       const std::unordered_set<size_t> &deactivated_clauses);
    /**
     * Negates an assignment value ("unassigned" stays unchanged).
     */
    static Assignment negate(const Assignment &a);

    /**
     * Adds a new clause.
     *
     * This causes a backtrack to 0th level.
     * Propagates when finished.
     */
    void add_clause(const Clause &clause, bool active = true);
    /**
     * Adds new clauses, with some of them deactivated by default.
     *
     * This causes a backtrack to 0th level.
     * Propagates when finished.
     */
    void add_clauses(const std::vector<Clause> &clauses, const std::unordered_set<size_t> &deactivated_clauses);

    /**
     * @return true if contradiction was derived.
     */
    bool contains_empty() const;
    /**
     * @return current assignment level (number of explicitly assigned - not implied - variables).
     */
    size_t get_assignment_level() const;
    /**
     * Assigns a literal to be true and propagates.
     * @return true if propagation was successful, false if contradiction was derived.
     */
    bool assign_value(Literal l);
    /**
     * @return Current assignment of given literal.
     */
    Assignment get_assignment(Literal l) const;
    /**
     * Backtracks by a given number of levels.
     *
     * Throws if current assignment level is not high enough.
     */
    void backtrack(size_t num_levels);
    /**
     * Backtracks to a given assignment level.
     *
     * Throws if the given level is higher than current assignment level.
     */
    void backtrack_to(size_t target_level);
    /**
     * Activates and/or deactivates given clauses.
     *
     * This causes a backtrack to 0th level.
     * Propagates when finished.
     */
    void change_active_clauses(const std::vector<size_t> &activate_indices,
                               const std::vector<size_t> &deactivate_indices);
    /**
     * Prints clauses in the data structure and their current watches.
     */
    void print_clauses(std::ostream &os = std::cout) const;
    /**
     * Prints current assignment stack.
     */
    void print_stack(std::ostream &os = std::cout) const;

private:
    using StackElement = std::vector<Literal>;

    struct ClauseData {
        const Clause &clause;
        size_t watched1{0};
        size_t watched2{0};
        bool is_active{false};

        explicit ClauseData(const Clause &c) : clause(c) {}
    };

    struct VarData {
        std::unordered_set<size_t> watched_clauses_positive{};
        std::unordered_set<size_t> watched_clauses_negative{};
        Assignment assignment{Assignment::unassigned};
    };

    const size_t m_max_var;
    std::vector<ClauseData> m_clauses;
    std::vector<VarData> m_variables;
    std::vector<StackElement> m_stack;
    std::unordered_set<size_t> m_unit_clauses;
    std::unordered_set<size_t> m_initial_unit_clauses;
    size_t m_empty_count{0};
    size_t m_initial_empty_count{0};

    void init();
    void add_clause_impl(const Clause &clause, bool active);
    void activate_clause(size_t clause_index, bool skip_if_active);
    void deactivate_clause(size_t clause_index, bool skip_if_not_active);
    bool propagate();
    bool assign_value_impl(Literal l);
    bool update_watched_literal(size_t clause_index, size_t var_index);
    void backtrack_impl();

    static size_t get_var_index(Literal l);

    static size_t find_max_var(const std::vector<Clause> &clauses);
};

} // namespace dp
