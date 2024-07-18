#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <deque>
#include <iostream>
#include <functional>
#include <memory>
#include <cstdio>

#include "utils.hpp"
#include "data_structures/lru_cache.hpp"

namespace dp {

/**
 * Exception thrown when the Sylvan library's unique table is full. No recovery possible.
 */
class SylvanFullTableException : public std::runtime_error {
public:
    SylvanFullTableException(const std::string &what) : std::runtime_error(what) {}
};

/**
 * CNF formula represented by a ZBDD implemented with the Sylvan library.
 *
 * Instances are immutable, all operations create new ZBDDs without modifying the operands.
 * Assumes that Lace threads are always suspended outside of the class.
 */
class SylvanZddCnf {
public:
    /**
     * Represents a literal of a propositional variable.
     * Positive literals are positive numbers and vice versa.
     * 0 is an invalid literal.
     */
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using ClauseFunction = std::function<bool(const Clause &)>;

    /**
     * Contains positive and negative occurrence counts of a variable.
     */
    struct VariableStats {
        size_t positive_clause_count{0};
        size_t negative_clause_count{0};
    };

    /**
     * Contains occurrence counts of each variable in a formula.
     */
    struct FormulaStats {
        /**
         * Note: may contain holes (variables with 0 occurrences).
         */
        std::vector<VariableStats> vars;
        /**
         * Variable at the 0-th index (to save space if many low variables are missing).
         */
        size_t index_shift;
    };

    /**
     * Contains Sylvan unique table occupancy statistics.
     */
    struct SylvanStats {
        /**
         * Number of entries in the table.
         */
        size_t table_filled;
        /**
         * Current capacity of the table.
         */
        size_t table_total;
    };

    /**
     * Collects statistics about Sylvan's unique table.
     */
    static SylvanStats get_sylvan_stats();
    /**
     * Explicitly calls Sylvan's garbage collection.
     */
    static void call_sylvan_gc();
    /**
     * Adds hooks for Sylvan garbage collection (before and after).
     * Should be called after initialising Sylvan.
     */
    static void hook_sylvan_gc_log();

    /**
     * Creates an empty formula.
     */
    SylvanZddCnf();
    SylvanZddCnf(const SylvanZddCnf &other);
    SylvanZddCnf &operator=(const SylvanZddCnf &other);
    ~SylvanZddCnf();

    /**
     * Creates a ZBDD representation of a CNF formula from a vector of clauses.
     */
    static SylvanZddCnf from_vector(const std::vector<Clause> &clauses);
    /**
     * Creates a ZBDD representation of a CNF formula from a DIMACS CNF file.
     */
    static SylvanZddCnf from_file(const std::string &file_name);

    inline bool operator==(const SylvanZddCnf &other) const {
        return m_zdd == other.m_zdd;
    }

    [[nodiscard]] size_t count_clauses() const;
    /**
     * Counts the number of ZBDD nodes used by the formula.
     */
    [[nodiscard]] size_t count_nodes() const;
    /**
     * Measures the depth (longest path from root to terminal) of the ZBDD.
     */
    [[nodiscard]] size_t count_depth() const;
    /**
     * @return true if the formula is empty.
     */
    [[nodiscard]] bool is_empty() const;
    /**
     * @return true if the formula contains the empty clause.
     */
    [[nodiscard]] bool contains_empty() const;
    /**
     * @return true if the formula contains a unit clause with the given literal.
     */
    [[nodiscard]] bool contains_unit_literal(const Literal &literal) const;
    /**
     * @return Positive literal of the smallest variable in the formula.
     */
    [[nodiscard]] Literal get_smallest_variable() const;
    /**
     * @return Positive literal of the largest variable in the formula.
     */
    [[nodiscard]] Literal get_largest_variable() const;
    /**
     * @return Literal in the root of the ZBDD.
     */
    [[nodiscard]] Literal get_root_literal() const;
    /**
     * @return Literal in a unit clause of the formula if some exists, otherwise 0.
     */
    [[nodiscard]] Literal get_unit_literal() const;
    /**
     * @return Literal with only positive occurrences in the formula if it exists, otherwise 0.
     */
    [[nodiscard]] Literal get_clear_literal() const;
    /**
     *  Finds all literals and returns statistics containing 1 for each literal that occurs in the formula and 0 if it
     *  does not occur.
     */
    [[nodiscard]] FormulaStats find_all_literals() const;
    /**
     * Counts occurrences of all literals in the formula.
     * More expensive than find_all_literals().
     */
    [[nodiscard]] FormulaStats count_all_literals() const;
    /**
     * Computes the subset0 (offset) operation on the ZBDD for a given literal.
     */
    [[nodiscard]] SylvanZddCnf subset0(Literal l) const;
    /**
     * Computes the subset1 (onset) operation on the ZBDD for a given literal.
     */
    [[nodiscard]] SylvanZddCnf subset1(Literal l) const;
    /**
     * Computes the union with another formula.
     */
    [[nodiscard]] SylvanZddCnf unify(const SylvanZddCnf &other) const;
    /**
     * Computes the union with another formula while removing all subsumed clauses from the result.
     */
    [[nodiscard]] SylvanZddCnf unify_and_remove_subsumed(const SylvanZddCnf &other) const;
    /**
     * Computes the intersection with another formula.
     */
    [[nodiscard]] SylvanZddCnf intersect(const SylvanZddCnf &other) const;
    /**
     * Subtracts another formula from this formula.
     */
    [[nodiscard]] SylvanZddCnf subtract(const SylvanZddCnf &other) const;
    /**
     * Computes the subsumed difference of this formula and another formula.
     * The result contains clauses from this formula that are not subsumed by any clause in the other formula.
     */
    [[nodiscard]] SylvanZddCnf subtract_subsumed(const SylvanZddCnf &other) const;
    /**
     * Computes the product with another formula, distributing all their clauses over each other.
     */
    [[nodiscard]] SylvanZddCnf multiply(const SylvanZddCnf &other) const;
    /**
     * Removes all tautologies from the formula.
     */
    [[nodiscard]] SylvanZddCnf remove_tautologies() const;
    /**
     * Removes all subsumptions from the formula.
     */
    [[nodiscard]] SylvanZddCnf remove_subsumed_clauses() const;
    /**
     * Iterates through all clauses in the formula, calling the given callback function for each clause.
     * The iteration stops if the callback returns false.
     */
    void for_all_clauses(ClauseFunction &func) const;

    /**
     * @return Vector of clauses in the formula.
     */
    [[nodiscard]] std::vector<Clause> to_vector() const;

    /**
     * Prints all clauses in the formula to a given stream.
     */
    void print_clauses(std::ostream &output = std::cout) const;
    /**
     * Draws the underlying ZBDD in the DOT format to a given file pointer.
     */
    void draw_to_file(std::FILE *file) const;
    /**
     * Draws the underlying ZBDD in the DOT format to a given file.
     */
    void draw_to_file(const std::string &file_name) const;
    /**
     * Writes the formula in the DIMACS CNF format to a given file.
     */
    void write_dimacs_to_file(const std::string &file_name) const;

    /**
     * Debugging function that verifies correct variable ordering of the ZBDD.
     */
    [[nodiscard]] bool verify_variable_ordering() const;

private:
    using ZDD = uint64_t;
    /**
     * Variable in a ZBDD, not a propositional variable.
     */
    using Var = uint32_t;
    using NodeFunction = std::function<bool(const ZDD &)>;

    ZDD m_zdd;

    explicit SylvanZddCnf(ZDD zdd);

    // lace framework RAII activator
    class LaceActivator {
    public:
        LaceActivator();
        ~LaceActivator();

    private:
        static inline size_t s_depth{0};
    };

    // logarithmic building
    class LogarithmicBuilder {
    public:
        LogarithmicBuilder();
        ~LogarithmicBuilder();

        void add_clause(const Clause &c);
        [[nodiscard]] ZDD get_result() const;
        [[nodiscard]] size_t get_size() const;

    private:
        using entry = std::tuple<ZDD, size_t>;
        static constexpr size_t unit_size = 1024;
        std::deque<entry> m_forest;

        void check_and_merge();
    };

    // helper functions
    void for_all_nodes(NodeFunction &func) const;
    static Var literal_to_var(Literal l);
    static Literal var_to_literal(Var v);
    static ZDD clause_from_vector(const Clause &clause);
    static bool contains_empty_set(const ZDD &zdd);

    // implementations of recursive algorithms
    static bool for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack);
};

} // namespace dp
