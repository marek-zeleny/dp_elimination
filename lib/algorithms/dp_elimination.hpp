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
using SizeBasedCondition_f = std::function<bool(size_t iteration, size_t prev_cnf_size, size_t cnf_size)>;
using UnaryOperation_f = std::function<SylvanZddCnf(const SylvanZddCnf &cnf)>;
using UnaryOperationWithStopCondition_f = std::function<SylvanZddCnf(const SylvanZddCnf &cnf,
                                                                     const std::function<bool()> &stop_condition)>;
using BinaryOperation_f = std::function<SylvanZddCnf(const SylvanZddCnf &op1, const SylvanZddCnf &op2)>;
using BinaryOperationwithStopCondition_f = std::function<SylvanZddCnf(const SylvanZddCnf &op1,
                                                                      const SylvanZddCnf &op2,
                                                                      const std::function<bool()> &stop_condition)>;
using Heuristic_f = std::function<HeuristicResult(const SylvanZddCnf &cnf)>;
using IsAllowedVariable_f = std::function<bool(uint32_t var)>;

inline SylvanZddCnf default_union(const SylvanZddCnf &zdd1, const SylvanZddCnf &zdd2) {
    return zdd1.unify_and_remove_subsumed(zdd2);
}

[[nodiscard]]
inline SylvanZddCnf eliminate(const SylvanZddCnf &set, const SylvanZddCnf::Literal &l,
                              const BinaryOperation_f &unify = default_union) {
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
    auto timer_unify = metrics.get_timer(MetricsDurations::EliminateVar_Unification);
    SylvanZddCnf result = unify(without_l, no_tautologies);
    timer_unify.stop();

    return result;
}

inline long count_vars(const SylvanZddCnf &cnf) {
    const auto stats = cnf.count_all_literals();
    long var_count = std::count_if(stats.vars.cbegin(), stats.vars.cend(),
                                     [](const SylvanZddCnf::VariableStats &var) {
        return var.positive_clause_count > 0 || var.negative_clause_count > 0;
    });
    return var_count;
}

inline void log_zdd_stats(const size_t &num_clauses, const size_t &num_nodes, const size_t &depth) {
    if constexpr (simple_logger::Log<simple_logger::LogLevel::Debug>::isActive) {
        auto stats = SylvanZddCnf::get_sylvan_stats();
        LOG_DEBUG << "Sylvan table usage: " << stats.table_filled << "/" << stats.table_total;
    }
    LOG_INFO << "ZDD size - clauses: " << num_clauses << ", nodes: " << num_nodes << ", depth: " << depth;
}

struct EliminationAlgorithmConfig {
    Heuristic_f heuristic;
    StopCondition_f stop_condition;
    SizeBasedCondition_f complete_minimization_condition;
    UnaryOperationWithStopCondition_f complete_minimization;
    SizeBasedCondition_f partial_minimization_condition;
    SizeBasedCondition_f incremental_absorption_removal_condition;
    BinaryOperationwithStopCondition_f unify_and_remove_absorbed;
    IsAllowedVariable_f is_allowed_variable;
};

[[nodiscard]]
inline SylvanZddCnf eliminate_vars(SylvanZddCnf cnf, const EliminationAlgorithmConfig &config) {
    using unit_propagation::unit_propagation;
    assert(cnf.verify_variable_ordering());

    LOG_INFO << "Starting DP elimination algorithm";
    // initial metrics collection
    const auto clauses_count_start = static_cast<int64_t>(cnf.count_clauses());
    metrics.append_to_series(MetricsSeries::ClauseCounts, clauses_count_start);
    metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));
    metrics.increase_counter(MetricsCounters::InitVars, count_vars(cnf));
    auto timer = metrics.get_timer(MetricsDurations::AlgorithmTotal);

    log_zdd_stats(cnf.count_clauses(), cnf.count_nodes(), cnf.count_depth());

    // start by initial unit propagation
    std::unordered_set<SylvanZddCnf::Literal> removed_unit_literals = unit_propagation(cnf, true);
    assert(cnf.verify_variable_ordering());
    size_t clauses_count = cnf.count_clauses();

    // prepare for main loop
    size_t iter = 0;
    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = config.heuristic(cnf);
    timer_heuristic1.stop();

    // stop condition for minimization
    auto minimization_stop_condition = [&config, &iter, &cnf, &clauses_count, &result]() {
        return config.stop_condition(iter, cnf, clauses_count, result);
    };

    // helper lambda function for complete minimization
    size_t last_minimization_clauses_count = clauses_count;
    auto conditionally_minimize = [&config, &iter, &last_minimization_clauses_count, &clauses_count,
                                          &minimization_stop_condition](
            SylvanZddCnf &cnf) {
        if (minimization_stop_condition()) {
            return;
        }
        if (config.complete_minimization_condition(iter, last_minimization_clauses_count, clauses_count)) {
            cnf = config.complete_minimization(cnf, minimization_stop_condition);
            assert(cnf.verify_variable_ordering());
            clauses_count = cnf.count_clauses();
            last_minimization_clauses_count = clauses_count;
        }
    };

    // helper lambda function for subsumption removal and incremental absorption removal (during union)
    auto conditional_union = [&config, &iter, &minimization_stop_condition](
            const SylvanZddCnf &zdd1, const SylvanZddCnf &zdd2) {
        const size_t size1 = zdd1.count_clauses();
        const size_t size2 = zdd2.count_clauses();
        if (config.partial_minimization_condition(iter, size1, size2)) {
            if (config.incremental_absorption_removal_condition(iter, size1, size2)) {
                LOG_DEBUG << "Removing subsumed clauses from resolvents";
                SylvanZddCnf zdd2_reduced = zdd2.subtract_subsumed(zdd1).remove_subsumed_clauses();
                return config.unify_and_remove_absorbed(zdd1, zdd2_reduced, minimization_stop_condition);
            } else {
                LOG_DEBUG << "Computing subsumption-free union";
                return zdd1.unify_and_remove_subsumed(zdd2);
            }
        } else {
            return zdd1.unify(zdd2);
        }
    };

    // eliminate variables until a stop condition is met
    while (!config.stop_condition(iter, cnf, clauses_count, result)) {
        LOG_INFO << "Starting iteration #" << iter;
        metrics.append_to_series(MetricsSeries::HeuristicScores, result.score);

        cnf = eliminate(cnf, result.literal, conditional_union);
        assert(cnf.verify_variable_ordering());
        metrics.append_to_series(MetricsSeries::ClauseCountDifference,
                                 static_cast<int64_t>(cnf.count_clauses()) - static_cast<int64_t>(clauses_count));
        auto implied_literals = unit_propagation(cnf, true);
        removed_unit_literals.merge(implied_literals);
        assert(cnf.verify_variable_ordering());
        clauses_count = cnf.count_clauses();

        conditionally_minimize(cnf);

        log_zdd_stats(clauses_count, cnf.count_nodes(), cnf.count_depth());

        metrics.append_to_series(MetricsSeries::ClauseCounts, static_cast<int64_t>(clauses_count));
        metrics.append_to_series(MetricsSeries::NodeCounts, static_cast<int64_t>(cnf.count_nodes()));

        auto timer_heuristic2 = metrics.get_timer(MetricsDurations::VarSelection);
        result = config.heuristic(cnf);
        timer_heuristic2.stop();
        ++iter;
    }

    // re-insert removed unit clauses
    std::vector<SylvanZddCnf::Clause> returned_clauses;
    for (const auto &literal : removed_unit_literals) {
        if (!config.is_allowed_variable(std::abs(literal))) {
            returned_clauses.push_back({literal});
        }
    }
    if (!returned_clauses.empty()) {
        auto returned = SylvanZddCnf::from_vector(returned_clauses);
        cnf = cnf.unify(returned);
    }

    // final metrics collection
    timer.stop();
    metrics.increase_counter(MetricsCounters::FinalVars, count_vars(cnf));
    return cnf;
}

} // namespace dp
