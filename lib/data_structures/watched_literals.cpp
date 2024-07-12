#include "data_structures/watched_literals.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <iostream>
#include "metrics/dp_metrics.hpp"

namespace dp {

WatchedLiterals::WatchedLiterals(const std::vector<Clause> &clauses, size_t max_var,
                                 const std::unordered_set<size_t> &deactivated_clauses) : m_max_var(max_var) {
    // initialize variables
    m_variables.resize(m_max_var, {});
    // initialize clauses
    for (size_t i = 0; i < clauses.size(); ++i) {
        add_clause_impl(clauses[i], !deactivated_clauses.contains(i));
    }
    init();
}

WatchedLiterals::WatchedLiterals(const std::vector<Clause> &clauses, size_t max_var) :
        WatchedLiterals(clauses, max_var, {}) {}

WatchedLiterals WatchedLiterals::from_vector(const std::vector<Clause> &clauses,
                                             const std::unordered_set<size_t> &deactivated_clauses) {
    size_t max_var = find_max_var(clauses);
    return {clauses, max_var, deactivated_clauses};
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

void WatchedLiterals::add_clause(const dp::WatchedLiterals::Clause &clause, bool active) {
    // backtrack to empty stack
    backtrack_to(0);
    backtrack_impl();
    assert(m_stack.empty());
    // add new variables if needed
    size_t max_var = find_max_var({clause});
    if (max_var > m_variables.size()) {
        m_variables.resize(max_var, {});
    }
    // add clause and (de)activate
    add_clause_impl(clause, active);
    init();
}

void WatchedLiterals::add_clauses(const std::vector<Clause> &clauses,
                                  const std::unordered_set<size_t> &deactivated_clauses) {
    // backtrack to empty stack
    backtrack_to(0);
    backtrack_impl();
    assert(m_stack.empty());
    // add new variables if needed
    size_t max_var = find_max_var(clauses);
    if (max_var > m_variables.size()) {
        m_variables.resize(max_var, {});
    }
    // add clauses and (de)activate
    for (size_t i = 0; i < clauses.size(); ++i) {
        add_clause_impl(clauses[i], !deactivated_clauses.contains(i));
    }
    init();
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
    if (get_var_index(l) >= m_variables.size()) {
        return true;
    }
    if (get_assignment(l) != Assignment::unassigned) {
        throw std::invalid_argument("Cannot assign to an already assigned variable");
    }
    auto timer = metrics.get_cumulative_timer(MetricsCumulativeDurations::WatchedLiterals_Propagation);
    m_stack.emplace_back();
    bool success = assign_value_impl(l);
    if (!success) {
        return false;
    }
    success = propagate();
    return success;
}

WatchedLiterals::Assignment WatchedLiterals::get_assignment(Literal l) const {
    size_t idx = get_var_index(l);
    if (idx >= m_variables.size()) {
        return Assignment::unassigned;
    }
    Assignment a = m_variables[idx].assignment;
    return l > 0 ? a : negate(a);
}

void WatchedLiterals::backtrack(size_t num_levels) {
    size_t current_level = get_assignment_level();
    if (num_levels > current_level) {
        throw std::out_of_range("Trying to backtrack more levels than assignments made");
    }
    if (num_levels > 0) {
        m_unit_clauses.clear();
        m_empty_count = 0; // If the backtrack is valid, there mustn't have been any empty clauses pre-assignment
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
    for (auto &idx: activate_indices) {
        activate_clause(idx, true);
    }
    for (auto &idx: deactivate_indices) {
        deactivate_clause(idx, true);
    }
    init();
}

void WatchedLiterals::print_clauses(std::ostream &os) const {
    for (auto &clause_data: m_clauses) {
        os << "{";
        for (size_t i = 0; i < clause_data.clause.size(); ++i) {
            os << " " << clause_data.clause[i];
            if (clause_data.watched1 == i) {
                os << "*";
            }
            if (clause_data.watched2 == i) {
                os << "**";
            }
            os << ",";
        }
        os << "}";
        if (clause_data.is_active) {
            os << "&";
        }
        os << std::endl;
    }
}

void WatchedLiterals::print_stack(std::ostream &os) const {
    size_t i = 0;
    for (auto &level: m_stack) {
        for (auto &l: level) {
            os << l << "@" << i << " ";
        }
        ++i;
    }
}

void WatchedLiterals::init() {
    m_empty_count = m_initial_empty_count;
    m_unit_clauses = m_initial_unit_clauses;
    // propagate
    auto timer = metrics.get_cumulative_timer(MetricsCumulativeDurations::WatchedLiterals_Propagation);
    m_stack.emplace_back();
    if (!contains_empty()) {
        propagate();
    }
}

void WatchedLiterals::add_clause_impl(const dp::WatchedLiterals::Clause &clause, bool active) {
    const size_t idx = m_clauses.size();
    m_clauses.emplace_back(clause);
    ClauseData &data = m_clauses.back();
    data.watched1 = 0;
    data.watched2 = clause.size() <= 1 ? 0 : 1;
    if (active) {
        activate_clause(idx, false);
    } else {
        data.is_active = false;
    }
}

void WatchedLiterals::activate_clause(size_t clause_index, bool skip_if_active) {
    assert(clause_index < m_clauses.size());
    ClauseData &data = m_clauses[clause_index];
    if (skip_if_active && data.is_active) {
        return;
    }
    data.is_active = true;
    if (data.clause.empty()) {
        ++m_initial_empty_count;
    } else if (data.clause.size() == 1) {
        Literal l = data.clause[0];
        size_t var_idx = get_var_index(l);
        auto &watched = l > 0
                ? m_variables[var_idx].watched_clauses_positive
                : m_variables[var_idx].watched_clauses_negative;
        watched.insert(clause_index);
        m_initial_unit_clauses.insert(clause_index);
    } else {
        Literal l1 = data.clause[data.watched1];
        Literal l2 = data.clause[data.watched2];
        size_t var1_idx = get_var_index(l1);
        size_t var2_idx = get_var_index(l2);
        auto &watched1 = l1 > 0
                ? m_variables[var1_idx].watched_clauses_positive
                : m_variables[var1_idx].watched_clauses_negative;
        auto &watched2 = l2 > 0
                ? m_variables[var2_idx].watched_clauses_positive
                : m_variables[var2_idx].watched_clauses_negative;
        watched1.insert(clause_index);
        watched2.insert(clause_index);
    }
}

void WatchedLiterals::deactivate_clause(size_t clause_index, bool skip_if_not_active) {
    assert(clause_index < m_clauses.size());
    ClauseData &data = m_clauses[clause_index];
    if (skip_if_not_active && !data.is_active) {
        return;
    }
    data.is_active = false;
    if (data.clause.empty()) {
        --m_initial_empty_count;
    } else if (data.clause.size() == 1) {
        Literal l = data.clause[0];
        size_t var_idx = get_var_index(l);
        auto &watched = l > 0
                ? m_variables[var_idx].watched_clauses_positive
                : m_variables[var_idx].watched_clauses_negative;
        watched.erase(clause_index);
        m_initial_unit_clauses.erase(clause_index);
    } else {
        Literal l1 = data.clause[data.watched1];
        Literal l2 = data.clause[data.watched2];
        size_t var1_idx = get_var_index(l1);
        size_t var2_idx = get_var_index(l2);
        auto &watched1 = l1 > 0
                ? m_variables[var1_idx].watched_clauses_positive
                : m_variables[var1_idx].watched_clauses_negative;
        auto &watched2 = l2 > 0
                ? m_variables[var2_idx].watched_clauses_positive
                : m_variables[var2_idx].watched_clauses_negative;
        watched1.erase(clause_index);
        watched2.erase(clause_index);
    }
}

bool WatchedLiterals::propagate() {
    while (!m_unit_clauses.empty()) {
        // get a unit clause
        auto it = m_unit_clauses.begin();
        size_t clause_idx = *it;
        m_unit_clauses.erase(it);
        assert(clause_idx < m_clauses.size());
        ClauseData &clause_data = m_clauses[clause_idx];
        // obtain the literal to assign a value to
        Literal l1 = clause_data.clause[clause_data.watched1];
        Literal l2 = clause_data.clause[clause_data.watched2];
        Assignment a1 = get_assignment(l1);
        Assignment a2 = get_assignment(l2);
        Literal l;
        if (a1 == Assignment::positive || a2 == Assignment::positive) {
            // not a unit clause anymore, skip
            continue;
        } else if (a1 == Assignment::unassigned) {
            // check that the clause is really unit
            assert(a2 == Assignment::negative || clause_data.watched2 == clause_data.watched1);
            l = l1;
        } else {
            assert(a2 == Assignment::unassigned);
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
    assert(get_assignment(l) == Assignment::unassigned);
    metrics.increase_counter(MetricsCounters::WatchedLiterals_Assignments);
    size_t var_idx = get_var_index(l);
    VarData &var_data = m_variables[var_idx];
    var_data.assignment = l > 0 ? Assignment::positive : Assignment::negative;
    m_stack.back().push_back(l);
    // update literals that watched this variable (only opposite literals need to be checked)
    auto &watched = l > 0 ? var_data.watched_clauses_negative : var_data.watched_clauses_positive;
    auto it = watched.begin();
    while (it != watched.end()) {
        bool moved_to_new_literal = update_watched_literal(*it, var_idx);
        if (moved_to_new_literal) {
            it = watched.erase(it);
        } else {
            ++it;
        }
        // check if an empty clause was created
        if (contains_empty()) {
            return false;
        }
    }
    return true;
}

bool WatchedLiterals::update_watched_literal(size_t clause_index, size_t var_index) {
    assert(clause_index < m_clauses.size());
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
    assert(a2 == Assignment::unassigned);
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
        Literal l = clause_data.clause[w_new];
        Assignment a = get_assignment(l);
        switch (a) {
            case Assignment::negative:
                // continue search
                break;
            case Assignment::positive:
            case Assignment::unassigned:
                // satisfied clause or unassigned variable -> update in either case
                w1 = w_new;
                if (l > 0) {
                    m_variables[get_var_index(l)].watched_clauses_positive.insert(clause_index);
                } else {
                    m_variables[get_var_index(l)].watched_clauses_negative.insert(clause_index);
                }
                return true;
            default:
                throw std::logic_error("Unexpected enum value");
        }
    }
}

void WatchedLiterals::backtrack_impl() {
    auto timer = metrics.get_cumulative_timer(MetricsCumulativeDurations::WatchedLiterals_Backtrack);
    StackElement &assignments = m_stack.back();
    for (auto &literal: assignments) {
        m_variables[get_var_index(literal)].assignment = Assignment::unassigned;
    }
    m_stack.pop_back();
}

size_t WatchedLiterals::get_var_index(Literal l) {
    assert(l != 0);
    size_t idx = std::abs(l) - 1; // 0 variable doesn't exist
    return idx;
}

size_t WatchedLiterals::find_max_var(const std::vector<Clause> &clauses) {
    size_t max_var = 0;
    for (auto &clause: clauses) {
        for (auto &literal: clause) {
            assert(literal != 0);
            size_t var = std::abs(literal);
            max_var = std::max(max_var, var);
        }
    }
    return max_var;
}

} // namespace dp
