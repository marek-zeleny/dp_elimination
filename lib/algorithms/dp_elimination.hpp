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
using AbsorbedCondition_f = std::function<bool(bool in_main_loop, size_t iteration, size_t prev_cnf_size,
        size_t cnf_size)>;
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

    LOG_DEBUG << "Union";
    auto timer_unification = metrics.get_timer(MetricsDurations::EliminateVar_Unification);
    SylvanZddCnf result = no_tautologies.unify(without_l);
    timer_unification.stop();

    return result;
}

inline bool is_sat(SylvanZddCnf set, const Heuristic_f &heuristic = heuristics::SimpleHeuristic()) {
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

inline long count_vars(const SylvanZddCnf &cnf) {
    const auto stats = cnf.get_formula_statistics();
    long var_count = std::count_if(stats.vars.cbegin(), stats.vars.cend(),
                                     [](const SylvanZddCnf::VariableStats &var) {
        return var.positive_clause_count > 0 || var.negative_clause_count > 0;
    });
    return var_count;
}

struct EliminationAlgorithmConfig {
    Heuristic_f heuristic;
    StopCondition_f stop_condition;
    AbsorbedCondition_f remove_absorbed_condition;
    ClauseModifier_f remove_absorbed_clauses;
};

[[nodiscard]]
inline SylvanZddCnf eliminate_vars(SylvanZddCnf cnf, const EliminationAlgorithmConfig &config) {
    using unit_propagation::unit_propagation;

    LOG_INFO << "Starting DP elimination algorithm";
    // initial metrics collection
    const auto clauses_count_start = static_cast<int64_t>(cnf.count_clauses());
    metrics.append_to_series(MetricsSeries::ClauseCounts, clauses_count_start);
    metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));
    metrics.increase_counter(MetricsCounters::InitVars, count_vars(cnf));
    auto timer = metrics.get_timer(MetricsDurations::AlgorithmTotal);

    // helper lambda function for removing absorbed clauses
    size_t clauses_count;
    size_t last_absorbed_clauses_count = 0;
    size_t iter = 0;
    auto conditionally_remove_absorbed = [&](bool main_loop) {
        if (config.remove_absorbed_condition(main_loop, iter, last_absorbed_clauses_count, clauses_count)) {
            cnf = config.remove_absorbed_clauses(cnf);
            clauses_count = cnf.count_clauses();
            last_absorbed_clauses_count = clauses_count;
        }
    };

    // start by initial unit propagation and removing absorbed clauses
    cnf = unit_propagation(cnf, true);
    clauses_count = cnf.count_clauses();

    conditionally_remove_absorbed(false);

    // prepare for main loop
    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = config.heuristic(cnf);
    timer_heuristic1.stop();

    // eliminate variables until a stop condition is met
    while (!config.stop_condition(iter, cnf, clauses_count, result)) {
        metrics.append_to_series(MetricsSeries::HeuristicScores, result.score);

        cnf = eliminate(cnf, result.literal);
        metrics.append_to_series(MetricsSeries::ClauseCountDifference,
                                 static_cast<int64_t>(cnf.count_clauses()) - static_cast<int64_t>(clauses_count));
        cnf = unit_propagation(cnf, true);
        clauses_count = cnf.count_clauses();

        conditionally_remove_absorbed(true);

#ifndef NDEBUG // only for debug build
        SylvanZddCnf::call_sylvan_gc();
#endif

        if constexpr (simple_logger::Log<simple_logger::LogLevel::Debug>::isActive) {
            auto stats = SylvanZddCnf::get_sylvan_stats();
            LOG_DEBUG << "Sylvan table usage: " << stats.table_filled << "/" << stats.table_total;
        }
        LOG_INFO << "ZDD size - clauses: " << clauses_count << ", nodes: " << cnf.count_nodes()
                 << ", depth: " << cnf.count_depth();
        metrics.append_to_series(MetricsSeries::ClauseCounts, static_cast<int64_t>(clauses_count));
        metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));

        auto timer_heuristic2 = metrics.get_timer(MetricsDurations::VarSelection);
        result = config.heuristic(cnf);
        timer_heuristic2.stop();
        ++iter;
    }

    // clean up by removing absorbed clauses
    conditionally_remove_absorbed(false);

    // final metrics collection
    timer.stop();
    metrics.increase_counter(MetricsCounters::FinalVars, count_vars(cnf));
    return cnf;
}

} // namespace dp
