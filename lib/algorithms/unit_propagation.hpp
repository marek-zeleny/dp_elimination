#pragma once

#include <vector>
#include <unordered_set>
#include <iostream>
#include <cassert>
#include <simple_logger.h>

#include "data_structures/watched_literals.hpp"

namespace dp {

inline bool is_clause_absorbed(WatchedLiterals &formula, const std::vector<int32_t> &clause) {
    {
        GET_LOG_STREAM_DEBUG(log_stream);
        log_stream << "checking if clause {";
        for (auto &l: clause) {
            log_stream << l << ", ";
        }
        log_stream << "} is absorbed\n";
        log_stream << "initial assignments: ";
        formula.print_stack(log_stream);
    }
    if (formula.contains_empty()) {
        return false;
    }
    for (auto &literal: clause) {
        formula.backtrack_to(0);
        LOG_DEBUG << "checking if literal " << literal << " is empowered";
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
    return true;
}

inline std::vector<std::vector<int32_t>> remove_absorbed_clauses(const std::vector<std::vector<int32_t>> &clauses) {
    LOG_INFO << "removing absorbed clauses, starting with " << clauses.size();
    std::unordered_set<size_t> deactivated{0};
    //WatchedLiterals watched = WatchedLiterals::from_vector(clauses, deactivated);
    WatchedLiterals watched = WatchedLiterals::from_vector(clauses);
    //watched.print_clauses(log);

    std::vector<std::vector<int32_t>> output;
    std::vector<size_t> clauses_to_reactivate;
    LOG_DEBUG << "testing clause 0";
    watched.change_active_clauses(clauses_to_reactivate, {0});
    clauses_to_reactivate.push_back(0);
    LOG_DEBUG << "active clauses changed";
    //watched.print_clauses(log);
    if (is_clause_absorbed(watched, clauses[0])) {
        clauses_to_reactivate.pop_back();
    } else {
        output.push_back(clauses[0]);
    }
    for (size_t i = 1; i < clauses.size(); ++i) {
        LOG_DEBUG << "testing clause " << i;
        watched.change_active_clauses(clauses_to_reactivate, {i});
        clauses_to_reactivate.clear();
        clauses_to_reactivate.push_back(i);
        LOG_DEBUG << "active clauses changed";
        //watched.print_clauses(log);
        if (is_clause_absorbed(watched, clauses[i])) {
            clauses_to_reactivate.pop_back();
        } else {
            output.push_back(clauses[i]);
        }
    }
    LOG_INFO << "absorbed clauses removed, " << output.size() << " remaining";
    return output;
}

} // namespace dp
