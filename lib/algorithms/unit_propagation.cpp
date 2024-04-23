#include "algorithms/unit_propagation.hpp"

#include <unordered_set>
#include <iostream>
#include <cassert>
#include <simple_logger.h>

#include "metrics/dp_metrics.hpp"

namespace dp {

namespace unit_propagation {

// Unit propagation on ZDD
void unit_propagation_step(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &unit_literal) {
    SylvanZddCnf without_l = cnf.subset0(unit_literal);
    SylvanZddCnf with_not_l = cnf.subset1(-unit_literal);
    cnf = without_l.unify(with_not_l);
}

std::vector<SylvanZddCnf::Literal> unit_propagation(SylvanZddCnf &cnf, bool count_metrics) {
    LOG_DEBUG << "Running unit propagation";
    std::vector<SylvanZddCnf::Literal> removed_literals;
    SylvanZddCnf::Literal l = cnf.get_unit_literal();
    while (l != 0 && !cnf.contains_empty()) {
        unit_propagation_step(cnf, l);
        l = cnf.get_unit_literal();
        removed_literals.push_back(l);
    }
    if (count_metrics) {
        auto count = static_cast<int64_t>(removed_literals.size());
        metrics.increase_counter(MetricsCounters::UnitLiteralsRemoved, count);
        metrics.append_to_series(MetricsSeries::UnitLiteralsRemoved, count);
    }
    LOG_DEBUG << "Unit propagation complete, removed " << removed_literals.size() << " unit clauses";
    return removed_literals;
}

bool unit_propagation_implies_literal(SylvanZddCnf &cnf, const SylvanZddCnf::Literal &stop_literal) {
    LOG_TRACE << "Checking if literal " << stop_literal << " is implied by unit propagation";
    SylvanZddCnf::Literal l = cnf.get_unit_literal();
    while (l != 0) {
        if (l == stop_literal || cnf.contains_empty() || cnf.contains_unit_literal(stop_literal)) {
            return true;
        } else if (l == -stop_literal || cnf.contains_unit_literal(-stop_literal)) {
            return false;
        }
        unit_propagation_step(cnf, l);
        l = cnf.get_unit_literal();
    }
    return false;
}

} // namespace unit_propagation

namespace absorbed_clause_detection {

// Absorbed clause detection on ZDD
bool is_clause_absorbed(const SylvanZddCnf &cnf, const SylvanZddCnf::Clause &clause) {
    using namespace unit_propagation;

    if (cnf.contains_empty()) {
        return false;
    }
    for (const auto &tested_literal: clause) {
        SylvanZddCnf curr = cnf;
        if (unit_propagation_implies_literal(curr, tested_literal)) {
            continue;
        }
        bool is_empowered = true;
        for (const auto &l: clause) {
            if (l == tested_literal) {
                continue;
            }
            unit_propagation_step(curr, -l);
            if (unit_propagation_implies_literal(curr, tested_literal)) {
                assert(curr.contains_empty() || curr.contains_unit_literal(tested_literal));
                is_empowered = false;
                break;
            }
        }
        if (is_empowered) {
            return false;
        }
    }
    GET_LOG_STREAM_TRACE(log);
    log << "found absorbed clause: {";
    for (const auto &l: clause) {
        log << l << ", ";
    }
    log << "}";
    return true;
}

SylvanZddCnf remove_absorbed_clauses_without_conversion(const SylvanZddCnf &cnf) {
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

// Absorbed clause detection on watched literals
bool is_clause_absorbed(WatchedLiterals &formula, const SylvanZddCnf::Clause &clause) {
    if (formula.contains_empty()) {
        return false;
    }
    for (auto &literal: clause) {
        formula.backtrack_to(0);
        // First we need to check if the potentially empowered literal is already positive
        if (formula.get_assignment(literal) == WatchedLiterals::Assignment::positive) {
            continue;
        }
        bool is_empowered = true;
        for (auto &l: clause) {
            if (l == literal) {
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
            if (empty_clause_created) {
                assert(formula.contains_empty());
                is_empowered = false;
                break;
            }
            if (formula.get_assignment(literal) == WatchedLiterals::Assignment::positive) {
                is_empowered = false;
                break;
            }
        }
        if (is_empowered) {
            return false;
        }
    }
    GET_LOG_STREAM_TRACE(log_stream);
    log_stream << "found absorbed clause: {";
    for (auto &l: clause) {
        log_stream << l << ", ";
    }
    log_stream << "}";
    return true;
}

std::vector<SylvanZddCnf::Clause> remove_absorbed_clauses(const std::vector<SylvanZddCnf::Clause> &clauses) {
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

SylvanZddCnf remove_absorbed_clauses_with_conversion(const SylvanZddCnf &cnf) {
    if (cnf.is_empty() || cnf.contains_empty()) {
        LOG_DEBUG << "Empty formula or clause, skipping absorbed detection";
        return cnf;
    }
    LOG_DEBUG << "Serializing ZDD into vector";
    auto timer_serialize = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Serialize);
    std::vector<SylvanZddCnf::Clause> vector = cnf.to_vector();
    timer_serialize.stop();

    auto timer_search = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Search);
    vector = remove_absorbed_clauses(vector);
    timer_search.stop();

    LOG_DEBUG << "Building ZDD from vector";
    auto timer_build = metrics.get_timer(MetricsDurations::RemoveAbsorbedClauses_Build);
    return SylvanZddCnf::from_vector(vector);
}

} // namespace absorbed_clause_detection

} // namespace dp
