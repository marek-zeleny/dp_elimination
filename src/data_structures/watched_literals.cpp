#include "data_structures/watched_literals.hpp"

#include <limits>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace dp {

WatchedLiterals::WatchedLiterals(const std::vector<Clause> &clauses, size_t min_var, size_t max_var,
    const std::unordered_set<size_t> &deactivated_clauses)
    : m_min_var(min_var), m_max_var(max_var), m_empty_count(0) {
    // initialize variables
    m_variables.resize(m_max_var - m_min_var + 1, {});
    // initialize clauses
    for (size_t i = 0; i < clauses.size(); ++i) {
        const Clause &c = clauses[i];
        m_clauses.emplace_back(c);
        ClauseData &data = m_clauses.back();
        data.watched1 = 0;
        data.watched2 = c.size() <= 1 ? 0 : 1;
        if (deactivated_clauses.contains(i)) {
            data.is_active = false;
        } else {
            activate_clause(i, false);
        }
    }
    m_empty_count = m_initial_empty_count;
    m_unit_clauses = m_initial_unit_clauses;
    // propagate
    m_stack.push_back({});
    if (!contains_empty()) {
        propagate();
    }
}

WatchedLiterals::WatchedLiterals(const std::vector<Clause> &clauses, size_t min_var, size_t max_var)
    : WatchedLiterals(clauses, min_var, max_var, {}) {}

WatchedLiterals WatchedLiterals::from_vector(const std::vector<Clause> &clauses,
    const std::unordered_set<size_t> &deactivated_clauses) {
    size_t min_var = std::numeric_limits<size_t>::max();
    size_t max_var = std::numeric_limits<size_t>::min();
    for (auto && clause : clauses) {
        for (auto && literal : clause) {
            assert(literal != 0);
            size_t var = std::abs(literal);
            min_var = std::min(min_var, var);
            max_var = std::max(max_var, var);
        }
    }
    return WatchedLiterals(clauses, min_var, max_var, deactivated_clauses);
}

WatchedLiterals WatchedLiterals::from_vector(const std::vector<Clause> &clauses) {
    return from_vector(clauses, {});
}

WatchedLiterals::Assignment WatchedLiterals::negate(const Assignment &a) {
    switch (a) {
        case Assignment::unassigned:    return Assignment::unassigned;
        case Assignment::negative:      return Assignment::positive;
        case Assignment::positive:      return Assignment::negative;
        default: throw std::logic_error("Unexpected enum value");
    }
}

bool WatchedLiterals::contains_empty() const {
    return m_empty_count > 0;
}

size_t WatchedLiterals::get_assignment_level() const {
    assert(m_stack.size() > 0);
    return m_stack.size() - 1;
}

bool WatchedLiterals::assign_value(Literal l) {
    if (contains_empty()) {
        return false;
    }
    m_stack.push_back({});
    bool success = assign_value_impl(l);
    if (!success) {
        return false;
    }
    success = propagate();
    return success;
}

WatchedLiterals::Assignment WatchedLiterals::get_assignment(Literal l) const {
    Assignment a = m_variables[get_var_index(l)].assignment;
    return l > 0 ? a : negate(a);
}

void WatchedLiterals::backtrack(size_t num_levels) {
    size_t current_level = get_assignment_level();
    std::cout << "backtracking " << num_levels << "/" << current_level << " levels" << std::endl;
    std::cout << "stack: ";
    print_stack();
    if (num_levels > current_level) {
        throw std::out_of_range("Trying to backtrack more levels than assignments made");
    }
    if (num_levels > 0) {
        m_empty_count = m_initial_empty_count;
    }
    for (; num_levels > 0; --num_levels) {
        backtrack_impl();
    }
}

void WatchedLiterals::backtrack_to(size_t target_level) {
    size_t current_level = get_assignment_level();
    if (target_level > current_level) {
        throw std::out_of_range("Trying to backtrack to level higher than assignments made");
    }
    backtrack(current_level - target_level);
}

void WatchedLiterals::change_active_clauses(const std::vector<size_t> &activate_indices,
    const std::vector<size_t> &deactivate_indices) {
    // backtrack to empty stack
    backtrack_to(0);
    backtrack_impl();
    assert(m_stack.empty());
    // update active clauses
    for (auto &&idx : activate_indices) {
        activate_clause(idx, true);
    }
    for (auto &&idx : deactivate_indices) {
        deactivate_clause(idx, true);
    }
    m_empty_count = m_initial_empty_count;
    m_unit_clauses = m_initial_unit_clauses;
    // propagate
    m_stack.push_back({});
    if (!contains_empty()) {
        propagate();
    }
}

void WatchedLiterals::print_clauses() const {
    for (auto &&clause_data : m_clauses) {
        std::cout << "{";
        for (size_t i = 0; i < clause_data.clause.size(); ++i) {
            std::cout << " " << clause_data.clause[i];
            if (clause_data.watched1 == i) {
                std::cout << "*";
            }
            if (clause_data.watched2 == i) {
                std::cout << "**";
            }
            std::cout << ",";
        }
        std::cout << "}";
        if (clause_data.is_active) {
            std::cout << "&";
        }
        std::cout << std::endl;
    }
}

void WatchedLiterals::print_stack() const {
    size_t i = 0;
    for (auto &level : m_stack) {
        for (auto &l : level)  {
            std::cout << l << "@" << i << " ";
        }
        ++i;
    }
    std::cout << std::endl;
}

void WatchedLiterals::activate_clause(size_t clause_index, bool skip_if_active) {
    ClauseData &data = m_clauses[clause_index];
    if (skip_if_active && data.is_active) {
        return;
    }
    data.is_active = true;
    if (data.clause.size() == 0) {
        ++m_initial_empty_count;
    } else if (data.clause.size() == 1) {
        size_t var_idx = get_var_index(data.clause[0]);
        m_variables[var_idx].watched_clauses.insert(clause_index);
        m_initial_unit_clauses.insert(clause_index);
    } else {
        size_t var1_idx = get_var_index(data.clause[data.watched1]);
        size_t var2_idx = get_var_index(data.clause[data.watched2]);
        m_variables[var1_idx].watched_clauses.insert(clause_index);
        m_variables[var2_idx].watched_clauses.insert(clause_index);
    }
}

void WatchedLiterals::deactivate_clause(size_t clause_index, bool skip_if_not_active) {
    ClauseData &data = m_clauses[clause_index];
    if (skip_if_not_active && !data.is_active) {
        return;
    }
    data.is_active = false;
    if (data.clause.size() == 0) {
        --m_initial_empty_count;
    } else if (data.clause.size() == 1) {
        size_t var_idx = get_var_index(data.clause[0]);
        m_variables[var_idx].watched_clauses.erase(clause_index);
        m_initial_unit_clauses.erase(clause_index);
    } else {
        size_t var1_idx = get_var_index(data.clause[data.watched1]);
        size_t var2_idx = get_var_index(data.clause[data.watched2]);
        m_variables[var1_idx].watched_clauses.erase(clause_index);
        m_variables[var2_idx].watched_clauses.erase(clause_index);
    }
}

bool WatchedLiterals::propagate() {
    while (!m_unit_clauses.empty()) {
        // get a unit clause
        auto it = m_unit_clauses.begin();
        size_t clause_idx = *it;
        m_unit_clauses.erase(it);
        ClauseData &clause_data = m_clauses[clause_idx];
        // obtain the literal to assign a value to
        Literal l1 = clause_data.clause[clause_data.watched1];
        Literal l2 = clause_data.clause[clause_data.watched2];
        Literal l;
        if (get_assignment(l1) == Assignment::unassigned) {
            // check that the clause is really unit
            assert(clause_data.watched2 == clause_data.watched1 || get_assignment(l2) != Assignment::unassigned);
            l = l1;
        } else {
            assert(get_assignment(l2) == Assignment::unassigned);
            l = l2;
        }
        // assign value to corresponding variable
        bool success = assign_value_impl(l);
        // if assignment wasn't successful (empty clause detected), stop propagation
        if (!success) {
            return false;
        }
    }
    return true;
}

bool WatchedLiterals::assign_value_impl(Literal l) {
    size_t var_idx = get_var_index(l);
    VarData &var_data = m_variables[var_idx];
    var_data.assignment = l > 0 ? Assignment::positive : Assignment::negative;
    m_stack.back().push_back(l);
    // update literals that watched this variable
    std::unordered_set<size_t> removed_watches;
    for (auto &&idx : var_data.watched_clauses) {
        bool moved_to_new_literal = update_watched_literal(idx, var_idx);
        if (moved_to_new_literal) {
            removed_watches.insert(idx);
        }
        // check if an empty clause was created
        if (contains_empty()) {
            return false;
        }
    }
    var_data.watched_clauses.erase(removed_watches.cbegin(), removed_watches.cend());
    return true;
}

bool WatchedLiterals::update_watched_literal(size_t clause_index, size_t var_index) {
    ClauseData &clause_data = m_clauses[clause_index];
    size_t &w1 = clause_data.watched1;
    size_t &w2 = clause_data.watched2;
    size_t var1_idx = get_var_index(clause_data.clause[w1]);
    size_t var2_idx = get_var_index(clause_data.clause[w2]);
    // ensure w1 is a reference to the updated literal
    if (var1_idx != var_index) {
        // check that the clause really watches given variable
        assert(var2_idx == var_index);
        std::swap(w1, w2);
        std::swap(var1_idx, var2_idx);
    }
    Assignment a1 = get_assignment(clause_data.clause[w1]);
    Assignment a2 = get_assignment(clause_data.clause[w2]);
    // either watch is positive -> clause is satisfied, no update
    if (a1 == Assignment::positive || a2 == Assignment::positive) {
        return false;
    }
    // both watches are negative -> clause is empty (UNSAT)
    assert(a1 == Assignment::negative);
    if (a2 == Assignment::negative) {
        ++m_empty_count;
        return false;
    }
    // second watch is unassigned -> try to move the first watch
    // note that the previous 2 cases also cover clauses with only one literal -> we know that w1 != w2
    size_t w_new = w1;
    while (true) {
        // move to next unwatched literal
        ++w_new;
        if (w_new == w2) {
            ++w_new;
        }
        if (w_new >= clause_data.clause.size()) {
            w_new = w2 == 0 ? 1 : 0;
        }
        // got back to w1 -> unit clause, no update of watched literals
        if (w_new == w1) {
            m_unit_clauses.insert(clause_index);
            return false;
        }
        // found an unassigned variable -> update
        Literal l = clause_data.clause[w_new];
        if (get_assignment(l) == Assignment::unassigned) {
            w1 = w_new;
            m_variables[get_var_index(l)].watched_clauses.insert(clause_index);
            return true;
        }
        // otherwise continue search
    }
}

void WatchedLiterals::backtrack_impl() {
    StackElement &assignments = m_stack.back();
    for (auto &&literal : assignments) {
        m_variables[get_var_index(literal)].assignment = Assignment::unassigned;
    }
    m_stack.pop_back();
}

size_t WatchedLiterals::get_var_index(Literal l) const {
    return std::abs(l) - m_min_var;
}

} // namespace dp
