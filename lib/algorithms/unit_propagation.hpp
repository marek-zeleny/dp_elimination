#pragma once

#include <vector>
#include <unordered_set>
#include <iostream>
#include <cassert>
#include <simple_logger.h>

#include "data_structures/watched_literals.hpp"
#include "metrics/dp_metrics.hpp"

namespace dp {

inline bool is_clause_absorbed(WatchedLiterals &formula, const std::vector<int32_t> &clause) {
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
    {
        GET_LOG_STREAM_DEBUG(log_stream);
        log_stream << "found absorbed clause: {";
        for (auto &l: clause) {
            log_stream << l << ", ";
        }
        log_stream << "}";
    }
    return true;
}

inline std::vector<std::vector<int32_t>> remove_absorbed_clauses(const std::vector<std::vector<int32_t>> &clauses) {
    LOG_INFO << "Removing absorbed clauses, starting with " << clauses.size() << " clauses";
    metrics.increase_counter(MetricsCounters::RemoveAbsorbedClausesCallCount);
    std::unordered_set<size_t> deactivated{0};
    WatchedLiterals watched = WatchedLiterals::from_vector(clauses, deactivated);

    std::vector<std::vector<int32_t>> output;
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
    size_t removed_count = clauses.size() - output.size();
    metrics.increase_counter(MetricsCounters::AbsorbedClausesRemovedTotal, removed_count);
    metrics.append_to_series(MetricsSeries::AbsorbedClausesRemoved, removed_count);
    LOG_INFO << removed_count << " absorbed clauses removed, " << output.size() << " remaining";
    return output;
}

} // namespace dp
