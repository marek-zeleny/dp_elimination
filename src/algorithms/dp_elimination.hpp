# pragma once

#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

template<typename Set>
Set eliminate(const Set &set, const typename Set::Var &v) {
    typename Set::Literal l = Set::var_to_literal(v);
    Set with_l = set.filter_literal_in(l);
    Set with_not_l = set.filter_literal_in(-l);
    Set without_l = set.filter_literal_out(l).filter_literal_out(-l);

    Set resolvents = with_l.resolve_all_pairs(with_not_l, v);
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
        } else if (set.contains_empty_clause()) {
            return false;
        }
        typename Set::Var v = heuristic.get_next_variable(set);
        std::cout << "eliminating variable " << v << std::endl;
        set = eliminate(set, v);
    }
}

} // namespace dp
