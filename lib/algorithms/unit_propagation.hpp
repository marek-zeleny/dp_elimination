#pragma once

#include <vector>
#include <unordered_set>
#include <functional>

#include "data_structures/watched_literals.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

/**
 * Contains functions for unit propagation over ZBDDs.
 */
namespace unit_propagation {

/**
 * Assigns a positive value in a formula to a given literal.
 *
 * @param cnf Note that the given reference is modified.
 */
void unit_propagation_step(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &unit_literal);

/**
 * Performs unit propagation over a given formula.
 *
 * @param cnf Note that the given reference is modified.
 * @param count_metrics If true, includes the implied literals into collected metrics.
 * @return Set of literals implied by the unit propagation.
 */
std::unordered_set<SylvanZddCnf::Literal> unit_propagation(SylvanZddCnf &cnf, bool count_metrics = false);

} // namespace unit_propagation

/**
 * Contains functions for detecting absorbed clauses.
 */
namespace absorbed_clause_detection {

using StopCondition_f = std::function<bool()>;

/**
 * Default stop condition (never stop).
 */
inline bool no_stop_condition() {
    return false;
}

/**
 * Absorption detection directly over ZBDDs.
 *
 * This implementation was soon realized to be much slower than watched literals and is not up-to-date.
 * It is not reachable through the user interface of the dp application and only serves for documentation purposes.
 */
namespace without_conversion {

/**
 * Checks if a clause is absorbed by a given formula.
 */
[[nodiscard]]
bool is_clause_absorbed(const SylvanZddCnf &cnf, const SylvanZddCnf::Clause &clause);

/**
 * Removes absorbed clauses from a given formula and returns the result as a new formula.
 *
 * @param stop_condition Condition to be checked in each iteration. Once it returns true, the algorithm stops.
 */
[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf,
                                     const StopCondition_f &stop_condition = no_stop_condition);

} // namespace without_conversion

/**
 * Absorption detection over watched literals.
 */
namespace with_conversion {

/**
 * Checks if a clause is absorbed by clauses in a given watched literals instance.
 *
 * @param formula Modified during the check, but always backtracks to 0 before returning.
 */
[[nodiscard]]
bool is_clause_absorbed(WatchedLiterals &formula, const SylvanZddCnf::Clause &clause);

/**
 * Removes absorbed clauses from a given vector and returns the result as a new vector of clauses.
 *
 * @param stop_condition Condition to be checked in each iteration. Once it returns true, the algorithm stops.
 */
[[nodiscard]]
std::vector<SylvanZddCnf::Clause> remove_absorbed_clauses_impl(const std::vector<SylvanZddCnf::Clause> &clauses,
                                                               const StopCondition_f &stop_condition = no_stop_condition);

/**
 * Removes absorbed clauses from a given ZBDD formula and returns the result as a new ZBDD.
 *
 * @param stop_condition Condition to be checked in each iteration. Once it returns true, the algorithm stops.
 */
[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf,
                                     const StopCondition_f &stop_condition = no_stop_condition);

/**
 * Iterative subsumption-free union of two ZBDD formulas.
 *
 * Computes a union of given ZBDD formulas by iterating through the clauses in the second one and only adding them to
 * the result if they are not absorbed.
 *
 * @param stop_condition Condition to be checked in each iteration. Once it returns true, the algorithm stops.
 */
[[nodiscard]]
SylvanZddCnf unify_with_non_absorbed(const SylvanZddCnf &stable, const SylvanZddCnf &checked,
                                     const StopCondition_f &stop_condition = no_stop_condition);

} // namespace with_conversion

} // namespace absorbed_clause_detection

} // namespace dp
