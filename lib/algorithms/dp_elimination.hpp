#pragma once

#include <concepts>
#include <algorithm>
#include <simple_logger.h>
#include "algorithms/heuristics.hpp"
#include "algorithms/unit_propagation.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "metrics/dp_metrics.hpp"

namespace dp {

template<typename F>
concept IsStopCondition = requires(F f, size_t iteration, const SylvanZddCnf &cnf, const HeuristicResult &result) {
    { f(iteration, cnf, result) } -> std::same_as<bool>;
};

inline SylvanZddCnf eliminate(const SylvanZddCnf &set, const SylvanZddCnf::Literal &l) {
    LOG_INFO << "Eliminating literal " << l;
    metrics.increase_counter(MetricsCounters::EliminatedVars);
    metrics.append_to_series(MetricsSeries::EliminatedLiterals, std::abs(l));
    auto timer_total = metrics.get_timer(MetricsDurations::EliminateVar_Total);

    auto timer_decomposition = metrics.get_timer(MetricsDurations::EliminateVar_SubsetDecomposition);
    SylvanZddCnf with_l = set.subset1(l);
    SylvanZddCnf with_not_l = set.subset1(-l);
    SylvanZddCnf without_l = set.subset0(l).subset0(-l);
    timer_decomposition.stop();

    auto timer_resolution = metrics.get_timer(MetricsDurations::EliminateVar_Resolution);
    SylvanZddCnf resolvents = with_l.multiply(with_not_l);
    timer_resolution.stop();

    auto timer_tautologies = metrics.get_timer(MetricsDurations::EliminateVar_TautologiesRemoval);
    SylvanZddCnf no_tautologies = resolvents.remove_tautologies();
    timer_tautologies.stop();

    auto timer_subsumed1 = metrics.get_timer(MetricsDurations::EliminateVar_SubsumedRemoval1);
    SylvanZddCnf no_tautologies_or_subsumed = no_tautologies.remove_subsumed_clauses();
    timer_subsumed1.stop();

    auto timer_unification = metrics.get_timer(MetricsDurations::EliminateVar_Unification);
    SylvanZddCnf result = no_tautologies_or_subsumed.unify(without_l);
    timer_unification.stop();

    auto timer_subsumed2 = metrics.get_timer(MetricsDurations::EliminateVar_SubsumedRemoval2);
    SylvanZddCnf no_subsumed = result.remove_subsumed_clauses();
    timer_subsumed2.stop();

    return no_subsumed;
}

template<IsHeuristic Heuristic = heuristics::SimpleHeuristic>
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
    auto timer = metrics.get_timer(MetricsDurations::RemoveAbsorbedClausesWithConversion);
    std::vector<SylvanZddCnf::Clause> vector = cnf.to_vector();
    vector = remove_absorbed_clauses(vector);
    cnf = SylvanZddCnf::from_vector(vector);
}

template<IsHeuristic Heuristic, IsStopCondition StopCondition>
SylvanZddCnf eliminate_vars(SylvanZddCnf cnf, Heuristic heuristic, StopCondition stop_condition,
                            size_t absorbed_clauses_interval = 0) {
    LOG_INFO << "Starting DP elimination algorithm";
    // initial metrics collection
    const size_t clauses_count_start = cnf.clauses_count();
    metrics.increase_counter(MetricsCounters::TotalClauses, clauses_count_start);
    const auto init_stats = cnf.get_formula_statistics();
    const size_t var_count = std::count_if(init_stats.vars.cbegin(), init_stats.vars.cend(),
                                           [](const SylvanZddCnf::VariableStats &var){
        return var.positive_clause_count > 0 || var.negative_clause_count > 0;
    });
    metrics.increase_counter(MetricsCounters::TotalVars, var_count);
    auto timer = metrics.get_timer(MetricsDurations::EliminateVars);

    // start by removing absorbed clauses
    if (absorbed_clauses_interval > 0) {
        remove_absorbed_clauses_from_cnf(cnf);
    }
    size_t i = 0;

    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = heuristic(cnf);
    timer_heuristic1.stop();

    // eliminate variables until a stop condition is met
    while (!stop_condition(i, cnf, result)) {
        ++i;
        metrics.append_to_series(MetricsSeries::HeuristicScores, result.score);
        const auto prev_clauses_count = static_cast<int64_t>(cnf.clauses_count());

        cnf = eliminate(cnf, result.literal);
        if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval == 0)) {
            remove_absorbed_clauses_from_cnf(cnf);
        }

        const int64_t removed_clauses = prev_clauses_count - static_cast<int64_t>(cnf.clauses_count());
        metrics.append_to_series(MetricsSeries::RemovedClauses, removed_clauses);

        auto timer_heuristic2 = metrics.get_timer(MetricsDurations::VarSelection);
        result = heuristic(cnf);
    }
    // clean up by removing absorbed clauses (unless it was already done during the last iteration)
    if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval != 0)) {
        remove_absorbed_clauses_from_cnf(cnf);
    }

    // final metrics collection
    timer.stop();
    metrics.increase_counter(MetricsCounters::RemovedClauses, clauses_count_start - cnf.clauses_count());
    return cnf;
}

} // namespace dp
