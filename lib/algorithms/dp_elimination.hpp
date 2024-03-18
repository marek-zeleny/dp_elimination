#pragma once

#include <concepts>
#include <simple_logger.h>
#include "algorithms/heuristics.hpp"
#include "algorithms/unit_propagation.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

template<typename F>
concept IsStopCondition = requires(F f, size_t iteration, const SylvanZddCnf &cnf, const HeuristicResult &result) {
    { f(iteration, cnf, result) } -> std::same_as<bool>;
};

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

template<IsHeuristic Heuristic = SimpleHeuristic>
bool is_sat(SylvanZddCnf set, Heuristic heuristic = {}) {
    LOG_INFO << "Starting DP elimination algorithm";
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
        HeuristicResult result = heuristic(set);
        set = eliminate(set, result.literal);
    }
}

static inline void remove_absorbed_clauses_from_cnf(SylvanZddCnf &cnf) {
    if (cnf.is_empty() || cnf.contains_empty()) {
        return;
    }
    std::vector<SylvanZddCnf::Clause> vector = cnf.to_vector();
    vector = remove_absorbed_clauses(vector);
    cnf = SylvanZddCnf::from_vector(vector);
}

template<IsHeuristic Heuristic, IsStopCondition StopCondition>
SylvanZddCnf eliminate_vars(SylvanZddCnf cnf, Heuristic heuristic, StopCondition stop_condition,
                            size_t absorbed_clauses_interval = 0) {
    LOG_INFO << "Starting DP elimination algorithm";
    // start by removing absorbed clauses
    if (absorbed_clauses_interval > 0) {
        remove_absorbed_clauses_from_cnf(cnf);
    }
    size_t i = 0;
    HeuristicResult result = heuristic(cnf);
    // eliminate variables until a stop condition is met
    while (!stop_condition(i, cnf, result)) {
        ++i;
        cnf = eliminate(cnf, result.literal);
        if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval == 0)) {
            remove_absorbed_clauses_from_cnf(cnf);
        }
        result = heuristic(cnf);
    }
    // clean up by removing absorbed clauses (unless it was already done during the last iteration)
    if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval != 0)) {
        remove_absorbed_clauses_from_cnf(cnf);
    }
    return cnf;
}

} // namespace dp
