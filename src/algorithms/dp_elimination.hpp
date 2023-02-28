# pragma once

#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

template<typename Set>
Set eliminate(const Set &set, const typename Set::Literal &l) {
    Set with_l = set.subset1(l);
    Set with_not_l = set.subset1(-l);
    Set without_l = set.subset0(l).subset0(-l);

    Set resolvents = with_l.multiply(with_not_l);
    Set result = resolvents.unify(without_l);
    return result;
}

template<typename Set>
bool is_sat(Set set) {
    std::cout << "Starting DP elimination algorithm:" << std::endl;
    typename Set::Heuristic heuristic = Set::get_new_heuristic();
    while (true) {
        std::cout << "CNF:" << std::endl;
        set.print_clauses();
        if (set.is_empty()) {
            return true;
        } else if (set.contains_empty()) {
            return false;
        }
        typename Set::Literal l = heuristic.get_next_literal(set);
        std::cout << "eliminating literal " << l << std::endl;
        set = eliminate(set, l);
    }
}

template<typename Set>
Set eliminate_vars(Set set, size_t num_vars) {
    typename Set::Heuristic heuristic = Set::get_new_heuristic();
    for (size_t i = 0; i < num_vars; ++i) {
        if (set.is_empty() || set.contains_empty()) {
            return set;
        }
        typename Set::Literal l = heuristic.get_next_literal(set);
        set = eliminate(set, l);
    }
    return set;
}

} // namespace dp
