#pragma once

#include <concepts>
#include <algorithm>
#include <functional>
#include <simple_logger.h>
#include "algorithms/heuristics.hpp"
#include "algorithms/unit_propagation.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "metrics/dp_metrics.hpp"

namespace dp {

using StopCondition_f = std::function<bool(size_t iteration, const SylvanZddCnf &cnf, size_t cnf_size,
        const HeuristicResult &result)>;
using AbsorbedCondition_f = std::function<bool(bool in_main_loop, size_t iteration, size_t cnf_size)>;
using ClauseModifier_f = std::function<SylvanZddCnf(const SylvanZddCnf &cnf)>;
using Heuristic_f = std::function<HeuristicResult(const SylvanZddCnf &cnf)>;

[[nodiscard]]
inline SylvanZddCnf eliminate(const SylvanZddCnf &set, const SylvanZddCnf::Literal &l) {
    LOG_INFO << "Eliminating literal " << l;
    metrics.increase_counter(MetricsCounters::EliminatedVars);
    metrics.append_to_series(MetricsSeries::EliminatedLiterals, std::abs(l));
    auto timer_total = metrics.get_timer(MetricsDurations::EliminateVar_Total);

    LOG_DEBUG << "Decomposition";
    auto timer_decomposition = metrics.get_timer(MetricsDurations::EliminateVar_SubsetDecomposition);
    SylvanZddCnf with_l = set.subset1(l);
    SylvanZddCnf with_not_l = set.subset1(-l);
    SylvanZddCnf without_l = set.subset0(l).subset0(-l);
    timer_decomposition.stop();

    LOG_DEBUG << "Resolution";
    auto timer_resolution = metrics.get_timer(MetricsDurations::EliminateVar_Resolution);
    SylvanZddCnf resolvents = with_l.multiply(with_not_l);
    timer_resolution.stop();

    LOG_DEBUG << "Removing tautologies";
    auto timer_tautologies = metrics.get_timer(MetricsDurations::EliminateVar_TautologiesRemoval);
    SylvanZddCnf no_tautologies = resolvents.remove_tautologies();
    timer_tautologies.stop();

    auto timer_subsumed1 = metrics.get_timer(MetricsDurations::EliminateVar_SubsumedRemoval1);
    SylvanZddCnf no_tautologies_or_subsumed = no_tautologies;//.remove_subsumed_clauses();
    timer_subsumed1.stop();

    LOG_DEBUG << "Union";
    auto timer_unification = metrics.get_timer(MetricsDurations::EliminateVar_Unification);
    SylvanZddCnf result = no_tautologies_or_subsumed.unify(without_l);
    timer_unification.stop();

    auto timer_subsumed2 = metrics.get_timer(MetricsDurations::EliminateVar_SubsumedRemoval2);
    SylvanZddCnf no_subsumed = result;//.remove_subsumed_clauses();
    timer_subsumed2.stop();

    return no_subsumed;
}

bool is_sat(SylvanZddCnf set, const Heuristic_f &heuristic = heuristics::SimpleHeuristic()) {
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

struct EliminationAlgorithmConfig {
    Heuristic_f heuristic;
    StopCondition_f stop_condition;
    AbsorbedCondition_f remove_absorbed_condition;
    ClauseModifier_f remove_absorbed_clauses;
};

[[nodiscard]]
SylvanZddCnf eliminate_vars(SylvanZddCnf cnf, const EliminationAlgorithmConfig &config) {
    using unit_propagation::unit_propagation;

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
    cnf = unit_propagation(cnf);
    size_t clauses_count = cnf.count_clauses();
    size_t iter = 0;
    if (config.remove_absorbed_condition(false, 0, clauses_count)) {
        cnf = config.remove_absorbed_clauses(cnf);
    }

    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = config.heuristic(cnf);
    timer_heuristic1.stop();

    // eliminate variables until a stop condition is met
    while (!config.stop_condition(iter, cnf, clauses_count, result)) {
        ++iter;
        metrics.append_to_series(MetricsSeries::HeuristicScores, result.score);

        cnf = eliminate(cnf, result.literal);
        const auto new_clauses_count = static_cast<int64_t>(cnf.count_clauses());
        metrics.append_to_series(MetricsSeries::EliminatedClauses, clauses_count - new_clauses_count);
        clauses_count = new_clauses_count;

        cnf = unit_propagation(cnf);
        if (config.remove_absorbed_condition(true, iter, clauses_count)) {
            cnf = config.remove_absorbed_clauses(cnf);
            clauses_count = static_cast<int64_t>(cnf.count_clauses());
        }
#ifndef NDEBUG // only for debug build
        SylvanZddCnf::call_sylvan_gc();
#endif
        auto stats = SylvanZddCnf::get_sylvan_stats();
        LOG_INFO << "ZDD size - clauses: " << clauses_count << ", nodes: " << cnf.count_nodes() <<
                  ", depth: " << cnf.count_depth();
        LOG_DEBUG << "Sylvan table usage: " << stats.table_filled << "/" << stats.table_total;
        metrics.append_to_series(MetricsSeries::ClauseCounts, clauses_count);
        metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));

        auto timer_heuristic2 = metrics.get_timer(MetricsDurations::VarSelection);
        result = config.heuristic(cnf);
    }
    // clean up by removing absorbed clauses (unless it was already done during the last iteration)
    if (config.remove_absorbed_condition(false, iter, clauses_count)) {
        cnf = config.remove_absorbed_clauses(cnf);
        clauses_count = static_cast<int64_t>(cnf.count_clauses());
    }

    // final metrics collection
    timer.stop();
    const int64_t removed_clauses = clauses_count_start - clauses_count;
    metrics.increase_counter(MetricsCounters::RemovedClauses, removed_clauses);
    return cnf;
}

} // namespace dp
