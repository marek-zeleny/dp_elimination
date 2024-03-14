#pragma once

#include <simple_logger.h>
#include "algorithms/unit_propagation.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

inline SylvanZddCnf eliminate(const SylvanZddCnf &set, const SylvanZddCnf::Literal &l) {
    LOG_INFO << "Eliminating literal " << l;
    SylvanZddCnf with_l = set.subset1(l);
    SylvanZddCnf with_not_l = set.subset1(-l);
    SylvanZddCnf without_l = set.subset0(l).subset0(-l);

    SylvanZddCnf resolvents = with_l.multiply(with_not_l);
    SylvanZddCnf no_tautologies_or_subsumed = resolvents.remove_tautologies().remove_subsumed_clauses();
    SylvanZddCnf result = no_tautologies_or_subsumed.unify(without_l);
    SylvanZddCnf no_subsumed = result.remove_subsumed_clauses();
    return no_subsumed;
}

inline bool is_sat(SylvanZddCnf set) {
    LOG_INFO << "Starting DP elimination algorithm";
    SylvanZddCnf::Heuristic heuristic = SylvanZddCnf::get_new_heuristic();
    while (true) {
        {
            GET_LOG_STREAM_DEBUG(log_stream);
            log_stream << "CNF:\n";
            set.print_clauses(log_stream);
        }
        if (set.is_empty()) {
            return true;
        } else if (set.contains_empty()) {
            return false;
        }
        SylvanZddCnf::Literal l = heuristic.get_next_literal(set);
        set = eliminate(set, l);
    }
}

static void remove_absorbed_clauses_from_set(SylvanZddCnf &set) {
    std::vector<SylvanZddCnf::Clause> vector = set.to_vector();
    vector = remove_absorbed_clauses(vector);
    set = SylvanZddCnf::from_vector(vector);
}

inline SylvanZddCnf eliminate_vars(SylvanZddCnf set, size_t num_vars, size_t absorbed_clauses_interval = 0) {
    LOG_INFO << "Starting DP elimination algorithm";
    SylvanZddCnf::Heuristic heuristic = SylvanZddCnf::get_new_heuristic();
    for (size_t i = 0; i < num_vars; ++i) {
        if (set.is_empty() || set.contains_empty()) {
            return set;
        }
        SylvanZddCnf::Literal l = heuristic.get_next_literal(set);
        set = eliminate(set, l);
        if (absorbed_clauses_interval > 0 && i % absorbed_clauses_interval == 0) {
            remove_absorbed_clauses_from_set(set);
        }
    }
    if (absorbed_clauses_interval > 0) {
        remove_absorbed_clauses_from_set(set);
    }
    return set;
}

} // namespace dp
