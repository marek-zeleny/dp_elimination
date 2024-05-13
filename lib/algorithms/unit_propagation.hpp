#pragma once

#include <vector>
#include <unordered_set>
#include <functional>

#include "data_structures/watched_literals.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

namespace unit_propagation {

void unit_propagation_step(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &unit_literal);

std::unordered_set<SylvanZddCnf::Literal> unit_propagation(SylvanZddCnf &cnf, bool count_metrics = false);

} // namespace unit_propagation

namespace absorbed_clause_detection {

using StopCondition_f = std::function<bool()>;

inline bool no_stop_condition() {
    return false;
}

namespace without_conversion {

[[nodiscard]]
bool is_clause_absorbed(const SylvanZddCnf &cnf, const SylvanZddCnf::Clause &clause);

[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf,
                                     const StopCondition_f &stop_condition = no_stop_condition);

} // namespace without_conversion

namespace with_conversion {

[[nodiscard]]
bool is_clause_absorbed(WatchedLiterals &formula, const SylvanZddCnf::Clause &clause);

[[nodiscard]]
std::vector<SylvanZddCnf::Clause> remove_absorbed_clauses_impl(const std::vector<SylvanZddCnf::Clause> &clauses,
                                                               const StopCondition_f &stop_condition = no_stop_condition);

[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf,
                                     const StopCondition_f &stop_condition = no_stop_condition);

[[nodiscard]]
SylvanZddCnf unify_with_non_absorbed(const SylvanZddCnf &stable, const SylvanZddCnf &checked,
                                     const StopCondition_f &stop_condition = no_stop_condition);

} // namespace with_conversion

} // namespace absorbed_clause_detection

} // namespace dp
