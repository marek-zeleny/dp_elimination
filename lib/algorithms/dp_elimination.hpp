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
using AbsorbedCondition_f = std::function<bool(size_t iteration, size_t prev_cnf_size, size_t cnf_size)>;
using UnaryOperation_f = std::function<SylvanZddCnf(const SylvanZddCnf &cnf)>;
using BinaryOperation_f = std::function<SylvanZddCnf(const SylvanZddCnf &op1, const SylvanZddCnf &op2)>;
using Heuristic_f = std::function<HeuristicResult(const SylvanZddCnf &cnf)>;
using IsAllowedVariable_f = std::function<bool(uint32_t var)>;

inline SylvanZddCnf default_union(const SylvanZddCnf &zdd1, const SylvanZddCnf &zdd2) {
    return zdd1.unify(zdd2);
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
    const auto stats = cnf.get_formula_statistics();
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
    AbsorbedCondition_f remove_absorbed_condition;
    UnaryOperation_f remove_absorbed_clauses;
    AbsorbedCondition_f incrementally_remove_absorbed_condition;
    BinaryOperation_f unify_with_non_absorbed;
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

    // helper lambda function for removing absorbed clauses
    size_t iter = 0;
    size_t last_absorbed_clauses_count = clauses_count;
    auto conditionally_remove_absorbed = [&config, &iter, &last_absorbed_clauses_count, &clauses_count](
            SylvanZddCnf &cnf) {
        if (config.remove_absorbed_condition(iter, last_absorbed_clauses_count, clauses_count)) {
            cnf = config.remove_absorbed_clauses(cnf);
            assert(cnf.verify_variable_ordering());
            clauses_count = cnf.count_clauses();
            last_absorbed_clauses_count = clauses_count;
        }
    };

    // helper lambda function for incrementally removing absorbed clauses (during union)
    auto conditional_union = [&config, &iter](const SylvanZddCnf &zdd1, const SylvanZddCnf &zdd2) {
        if (config.incrementally_remove_absorbed_condition(iter, zdd1.count_clauses(), zdd2.count_clauses())) {
            return config.unify_with_non_absorbed(zdd1, zdd2);
        } else {
            return zdd1.unify(zdd2);
        }
    };

    // prepare for main loop
    auto timer_heuristic1 = metrics.get_timer(MetricsDurations::VarSelection);
    HeuristicResult result = config.heuristic(cnf);
    timer_heuristic1.stop();

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

        conditionally_remove_absorbed(cnf);

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
