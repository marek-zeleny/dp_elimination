#pragma once

#include <vector>
#include <concepts>

#include "data_structures/watched_literals.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

namespace unit_propagation {

void unit_propagation_step(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &unit_literal);

std::vector<SylvanZddCnf::Literal> unit_propagation(SylvanZddCnf &cnf, bool count_metrics = false);

[[nodiscard]]
bool unit_propagation_implies_literal(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &stop_literal);

} // namespace unit_propagation

namespace absorbed_clause_detection {

namespace without_conversion {

[[nodiscard]]
bool is_clause_absorbed(const SylvanZddCnf &cnf, const SylvanZddCnf::Clause &clause);

[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf);

} // namespace without_conversion

namespace with_conversion {

[[nodiscard]]
bool is_clause_absorbed(WatchedLiterals &formula, const SylvanZddCnf::Clause &clause);

[[nodiscard]]
std::vector<SylvanZddCnf::Clause> remove_absorbed_clauses_impl(const std::vector<SylvanZddCnf::Clause> &clauses);

[[nodiscard]]
SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf);

[[nodiscard]]
SylvanZddCnf unify_with_non_absorbed(const SylvanZddCnf &stable, const SylvanZddCnf &checked);

} // namespace with_conversion

} // namespace absorbed_clause_detection

} // namespace dp
