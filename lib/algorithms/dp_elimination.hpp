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
concept IsStopCondition = requires(F f, size_t iteration, const SylvanZddCnf &cnf, size_t cnf_size,
                                   const HeuristicResult &result) {
    { f(iteration, cnf, cnf_size, result) } -> std::same_as<bool>;
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
    SylvanZddCnf no_tautologies_or_subsumed = no_tautologies;//.remove_subsumed_clauses();
    timer_subsumed1.stop();

    auto timer_unification = metrics.get_timer(MetricsDurations::EliminateVar_Unification);
    SylvanZddCnf result = no_tautologies_or_subsumed.unify(without_l);
    timer_unification.stop();

    auto timer_subsumed2 = metrics.get_timer(MetricsDurations::EliminateVar_SubsumedRemoval2);
    SylvanZddCnf no_subsumed = result;//.remove_subsumed_clauses();
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
    const auto clauses_count_start = static_cast<int64_t>(cnf.count_clauses());
    metrics.increase_counter(MetricsCounters::TotalClauses, clauses_count_start);
    metrics.append_to_series(MetricsSeries::ClauseCounts, clauses_count_start);
    metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));
    const auto init_stats = cnf.get_formula_statistics();
    const int64_t var_count = std::count_if(init_stats.vars.cbegin(), init_stats.vars.cend(),
                                           [](const SylvanZddCnf::VariableStats &var){
        return var.positive_clause_count > 0 || var.negative_clause_count > 0;
    });
    metrics.increase_counter(MetricsCounters::TotalVars, var_count);
    auto timer = metrics.get_timer(MetricsDurations::EliminateVars);

    // start by removing absorbed clauses
    if (absorbed_clauses_interval > 0) {
        remove_absorbed_clauses_from_cnf(cnf);
    }
    auto clauses_count = static_cast<int64_t>(cnf.count_clauses());
    size_t i = 0;

    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = heuristic(cnf);
    timer_heuristic1.stop();

    // eliminate variables until a stop condition is met
    while (!stop_condition(i, cnf, clauses_count, result)) {
        ++i;
        metrics.append_to_series(MetricsSeries::HeuristicScores, result.score);

        cnf = eliminate(cnf, result.literal);
        if (!cnf.verify_variable_ordering()) {
            GET_LOG_STREAM_ERROR(log);
            log << "Invalid variable ordering";
            log.flush();
            cnf.draw_to_file("WRONG_ORDERING.dot");
        }
        const auto new_clauses_count = static_cast<int64_t>(cnf.count_clauses());
        metrics.append_to_series(MetricsSeries::EliminatedClauses, clauses_count - new_clauses_count);
        clauses_count = new_clauses_count;

        if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval == 0)) {
            remove_absorbed_clauses_from_cnf(cnf);
            clauses_count = static_cast<int64_t>(cnf.count_clauses());
        }

        sylvan_clear_and_mark();
        sylvan_clear_cache();
        auto stats = SylvanZddCnf::get_sylvan_stats();
        LOG_DEBUG << "ZDD size - clauses: " << clauses_count << ", nodes: " << cnf.count_nodes();
        LOG_DEBUG << "Sylvan table usage: " << stats.table_filled << "/" << stats.table_total;
        metrics.append_to_series(MetricsSeries::ClauseCounts, clauses_count);
        metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));

        auto timer_heuristic2 = metrics.get_timer(MetricsDurations::VarSelection);
        result = heuristic(cnf);
    }
    // clean up by removing absorbed clauses (unless it was already done during the last iteration)
    if ((absorbed_clauses_interval > 0) && (i % absorbed_clauses_interval != 0)) {
        remove_absorbed_clauses_from_cnf(cnf);
        clauses_count = static_cast<int64_t>(cnf.count_clauses());
    }

    // final metrics collection
    timer.stop();
    const int64_t removed_clauses = clauses_count_start - clauses_count;
    metrics.increase_counter(MetricsCounters::RemovedClauses, removed_clauses);
    return cnf;
}

} // namespace dp
