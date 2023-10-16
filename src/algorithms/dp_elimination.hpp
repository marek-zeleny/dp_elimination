#pragma once

#include "algorithms/unit_propagation.hpp"
#include "logging.hpp"

namespace dp {

template<typename Set>
Set eliminate(const Set &set, const typename Set::Literal &l) {
    Set with_l = set.subset1(l);
    Set with_not_l = set.subset1(-l);
    Set without_l = set.subset0(l).subset0(-l);

    Set resolvents = with_l.multiply(with_not_l);
    Set no_tautologies_or_subsumed = resolvents.remove_tautologies().remove_subsumed_clauses();
    Set result = no_tautologies_or_subsumed.unify(without_l);
    Set no_subsumed = result.remove_subsumed_clauses();
    return no_subsumed;
}

template<typename Set>
bool is_sat(Set set) {
    log << "Starting DP elimination algorithm:" << std::endl;
    typename Set::Heuristic heuristic = Set::get_new_heuristic();
    while (true) {
        log << "CNF:" << std::endl;
        set.print_clauses();
        if (set.is_empty()) {
            return true;
        } else if (set.contains_empty()) {
            return false;
        }
        typename Set::Literal l = heuristic.get_next_literal(set);
        log << "eliminating literal " << l << std::endl;
        set = eliminate(set, l);
    }
}

template<typename Set>
Set eliminate_vars(Set set, size_t num_vars, size_t absorbed_clauses_interval = 5) {
    typename Set::Heuristic heuristic = Set::get_new_heuristic();
    for (size_t i = 0; i < num_vars; ++i) {
        if (set.is_empty() || set.contains_empty()) {
            return set;
        }
        typename Set::Literal l = heuristic.get_next_literal(set);
        log << "eliminating literal " << l << std::endl;
        set = eliminate(set, l);
        if (i % absorbed_clauses_interval == absorbed_clauses_interval - 1) {
            std::vector<typename Set::Clause> vector = set.to_vector();
            vector = remove_absorbed_clauses(vector);
            set = Set::from_vector(vector);
        }
    }
    return set;
}

} // namespace dp
