#include "algorithms/unit_propagation.hpp"

#include <unordered_set>
#include <deque>
#include <iostream>
#include <cassert>
#include <simple_logger.h>

#include "metrics/dp_metrics.hpp"

namespace dp {

// Unit propagation on ZDD
namespace unit_propagation {

void unit_propagation_step(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &unit_literal) {
    SylvanZddCnf without_l = cnf.subset0(unit_literal);
    SylvanZddCnf without_l_and_not_l = without_l.subset0(-unit_literal);
    SylvanZddCnf without_l_with_not_l = without_l.subset1(-unit_literal);
    cnf = without_l_and_not_l.unify(without_l_with_not_l);
}

std::unordered_set<SylvanZddCnf::Literal> unit_propagation(SylvanZddCnf &cnf, bool count_metrics) {
    LOG_DEBUG << "Running unit propagation";
    std::unordered_set<SylvanZddCnf::Literal> implied_literals;
    SylvanZddCnf::Literal l = cnf.get_unit_literal();
    while (l != 0 && !cnf.contains_empty()) {
        unit_propagation_step(cnf, l);
        implied_literals.insert(l);
        l = cnf.get_unit_literal();
    }
    if (count_metrics) {
        auto count = static_cast<int64_t>(implied_literals.size());
        metrics.increase_counter(MetricsCounters::UnitLiteralsRemoved, count);
        metrics.append_to_series(MetricsSeries::UnitLiteralsRemoved, count);
    }
    LOG_DEBUG << "Unit propagation complete, implied " << implied_literals.size() << " unit literals";
    return std::move(implied_literals);
}

} // namespace unit_propagation

namespace absorbed_clause_detection {

// Absorbed clause detection on ZDD
namespace without_conversion {

bool is_clause_absorbed(const SylvanZddCnf &cnf, const SylvanZddCnf::Clause &clause) {
    namespace up = unit_propagation;

    if (cnf.contains_empty()) {
        return true;
    }
    SylvanZddCnf init_cnf = cnf;
    const std::unordered_set<SylvanZddCnf::Literal> init_implied_literals = up::unit_propagation(init_cnf);
    for (const auto &tested_literal: clause) {
        if (init_implied_literals.contains(tested_literal)) {
            continue;
        }
        SylvanZddCnf curr = init_cnf;
        std::unordered_set<SylvanZddCnf::Literal> implied_literals = init_implied_literals;
        bool is_empowered = true;
        for (const auto &l: clause) {
            if (l == tested_literal || implied_literals.contains(-l)) {
                continue;
            } else if (implied_literals.contains(l)) {
                is_empowered = false;
                break;
            }
            up::unit_propagation_step(curr, -l);
            implied_literals.insert(-l);
            auto curr_implied = up::unit_propagation(curr);
            if (curr.contains_empty() || curr_implied.contains(tested_literal)) {
                is_empowered = false;
                break;
            }
            implied_literals.merge(curr_implied);
        }
        if (is_empowered) {
            return false;
        }
    }
    if constexpr (simple_logger::Log<simple_logger::LogLevel::Trace>::isActive) {
        GET_LOG_STREAM_TRACE(log_stream);
        log_stream << "found absorbed clause: {";
        for (auto &l: clause) {
            log_stream << l << ", ";
        }
        log_stream << "}";
    }
    return true;
}

SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf) {
    LOG_INFO << "Removing absorbed clauses";
    metrics.increase_counter(MetricsCounters::RemoveAbsorbedClausesCallCount);
    auto timer = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Search);
    SylvanZddCnf output = cnf;
    int64_t removed_count = 0;
    SylvanZddCnf::ClauseFunction func = [&output, &removed_count](const SylvanZddCnf::Clause &c) {
        auto tested_clause = SylvanZddCnf::from_vector({c});
        SylvanZddCnf remaining = output.subtract(tested_clause);
        if (is_clause_absorbed(remaining, c)) {
            output = remaining;
            ++removed_count;
        }
        return true;
    };
    cnf.for_all_clauses(func);
    metrics.increase_counter(MetricsCounters::AbsorbedClausesRemoved, removed_count);
    metrics.append_to_series(MetricsSeries::AbsorbedClausesRemoved, removed_count);
    LOG_INFO << removed_count << " absorbed clauses removed";
    return output;
}

} // namespace without_conversion

// Absorbed clause detection on watched literals
namespace with_conversion {

bool is_clause_absorbed(WatchedLiterals &formula, const SylvanZddCnf::Clause &clause) {
    if (formula.contains_empty()) {
        return true;
    }
    for (auto &tested_literal: clause) {
        formula.backtrack_to(0);
        // First we need to check if the potentially empowered literal is already positive
        if (formula.get_assignment(tested_literal) == WatchedLiterals::Assignment::positive) {
            continue;
        }
        bool is_empowered = true;
        for (auto &l: clause) {
            if (l == tested_literal) {
                continue;
            }
            WatchedLiterals::Assignment a = formula.get_assignment(-l);
            if (a == WatchedLiterals::Assignment::negative) {
                is_empowered = false;
                break;
            } else if (a == WatchedLiterals::Assignment::positive) {
                continue;
            }
            bool empty_clause_created = !formula.assign_value(-l);
            if (empty_clause_created || formula.get_assignment(tested_literal) == WatchedLiterals::Assignment::positive) {
                is_empowered = false;
                break;
            }
        }
        if (is_empowered) {
            return false;
        }
    }
    if constexpr (simple_logger::Log<simple_logger::LogLevel::Trace>::isActive) {
        GET_LOG_STREAM_TRACE(log_stream);
        log_stream << "found absorbed clause: {";
        for (auto &l: clause) {
            log_stream << l << ", ";
        }
        log_stream << "}";
    }
    return true;
}

std::vector<SylvanZddCnf::Clause> remove_absorbed_clauses_impl(const std::vector<SylvanZddCnf::Clause> &clauses) {
    LOG_INFO << "Removing absorbed clauses, starting with " << clauses.size() << " clauses";
    metrics.increase_counter(MetricsCounters::RemoveAbsorbedClausesCallCount);
    std::unordered_set<size_t> deactivated{0};
    WatchedLiterals watched = WatchedLiterals::from_vector(clauses, deactivated);

    std::vector<SylvanZddCnf::Clause> output;
    std::vector<size_t> clauses_to_reactivate{0};
    if (is_clause_absorbed(watched, clauses[0])) {
        clauses_to_reactivate.pop_back();
    } else {
        output.push_back(clauses[0]);
    }
    for (size_t i = 1; i < clauses.size(); ++i) {
        watched.change_active_clauses(clauses_to_reactivate, {i});
        clauses_to_reactivate.clear();
        clauses_to_reactivate.push_back(i);
        if (is_clause_absorbed(watched, clauses[i])) {
            clauses_to_reactivate.pop_back();
        } else {
            output.push_back(clauses[i]);
        }
    }
    const auto removed_count = static_cast<int64_t>(clauses.size() - output.size());
    metrics.increase_counter(MetricsCounters::AbsorbedClausesRemoved, removed_count);
    metrics.append_to_series(MetricsSeries::AbsorbedClausesRemoved, removed_count);
    LOG_INFO << removed_count << " absorbed clauses removed, " << output.size() << " remaining";
    return output;
}

SylvanZddCnf remove_absorbed_clauses(const SylvanZddCnf &cnf) {
    if (cnf.is_empty() || cnf.contains_empty()) {
        LOG_DEBUG << "Empty formula or clause, skipping absorbed detection";
        return cnf;
    }
    LOG_DEBUG << "Serializing ZDD into vector";
    auto timer_serialize = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Serialize);
    std::vector<SylvanZddCnf::Clause> vector = cnf.to_vector();
    timer_serialize.stop();

    auto timer_search = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Search);
    vector = remove_absorbed_clauses_impl(vector);
    timer_search.stop();

    LOG_DEBUG << "Building ZDD from vector";
    auto timer_build = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Build);
    return SylvanZddCnf::from_vector(vector);
}

SylvanZddCnf unify_with_non_absorbed(const SylvanZddCnf &stable, const SylvanZddCnf &checked) {
    if (checked.is_empty()) {
        return stable;
    } else if (checked.contains_empty()) {
        return stable.unify(SylvanZddCnf::from_vector({{}}));
    }
    LOG_DEBUG << "Serializing ZDD into vector of watched literals";
    auto timer_serialize = metrics.get_timer(MetricsDurations::IncrementalAbsorbedRemoval_Serialize);
    std::vector<SylvanZddCnf::Clause> clauses = stable.to_vector();
    auto watched = WatchedLiterals::from_vector(clauses);
    timer_serialize.stop();

    size_t total_count = 0;
    std::deque<SylvanZddCnf::Clause> added_clauses; // needs to provide reference stability after push_back (not vector)
    SylvanZddCnf::ClauseFunction func = [&watched, &added_clauses, &total_count](const SylvanZddCnf::Clause &c) {
        ++total_count;
        if (!is_clause_absorbed(watched, c)) {
            added_clauses.emplace_back(c);
            watched.add_clause(added_clauses.back());
        }
        return true;
    };

    LOG_INFO << "Incrementally checking for absorbed clauses";
    auto timer_search = metrics.get_timer(MetricsDurations::IncrementalAbsorbedRemoval_Search);
    checked.for_all_clauses(func);
    timer_search.stop();
    LOG_INFO << "Unified with " << added_clauses.size() << "/" << total_count << " clauses, the rest were absorbed";

    const auto removed_count = static_cast<int64_t>(total_count - added_clauses.size());
    metrics.increase_counter(MetricsCounters::AbsorbedClausesNotAdded, removed_count);
    metrics.append_to_series(MetricsSeries::AbsorbedClausesNotAdded, removed_count);

    LOG_DEBUG << "Building ZDD from vector";
    auto timer_build = metrics.get_timer(MetricsDurations::IncrementalAbsorbedRemoval_Build);
    clauses.insert(clauses.end(), added_clauses.begin(), added_clauses.end());
    return SylvanZddCnf::from_vector(clauses);
}

} // namespace with_conversion

} // namespace absorbed_clause_detection

} // namespace dp
