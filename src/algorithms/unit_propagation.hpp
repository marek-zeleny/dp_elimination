#pragma once

#include <vector>
#include <unordered_set>
#include <iostream>

#include <cassert>
#include "data_structures/watched_literals.hpp"

namespace dp {

bool is_clause_absorbed(WatchedLiterals &formula, const std::vector<int32_t> &clause) {
    std::cout << "checking if clause {";
    for (auto &l : clause) {
        std::cout << l << ", ";
    }
    std::cout << "} is absorbed" << std::endl;
    std::cout << "initial assignments: ";
    formula.print_stack();
    if (formula.contains_empty()) {
        return false;
    }
    for (auto &literal : clause) {
        formula.backtrack_to(0);
        bool is_empowered = true;
        for (auto &l : clause) {
            if (l == literal) {
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

std::vector<std::vector<int32_t>> remove_absorbed_clauses(const std::vector<std::vector<int32_t>> &clauses) {
    std::cout << "removing absorbed clauses" << std::endl;
    std::unordered_set<size_t> deactivated {0};
    //WatchedLiterals watched = WatchedLiterals::from_vector(clauses, deactivated);
    WatchedLiterals watched = WatchedLiterals::from_vector(clauses);
    watched.print_clauses();

    std::vector<std::vector<int32_t>> output;
    std::vector<size_t> clauses_to_reactivate;
    std::cout << "testing clause 0" << std::endl;
    watched.change_active_clauses(clauses_to_reactivate, {0});
    clauses_to_reactivate.push_back(0);
    std::cout << "active clauses changed" << std::endl;
    watched.print_clauses();
    if (is_clause_absorbed(watched, clauses[0])) {
        clauses_to_reactivate.pop_back();
    } else {
        output.push_back(clauses[0]);
    }
    for (size_t i = 1; i < clauses.size(); ++i) {
        std::cout << "testing clause " << i << std::endl;
        watched.change_active_clauses(clauses_to_reactivate, {i});
        clauses_to_reactivate.clear();
        clauses_to_reactivate.push_back(i);
        std::cout << "active clauses changed" << std::endl;
        watched.print_clauses();
        if (is_clause_absorbed(watched, clauses[i])) {
            clauses_to_reactivate.pop_back();
        } else {
            output.push_back(clauses[i]);
        }
    }
    return output;
}

} // namespace dp
