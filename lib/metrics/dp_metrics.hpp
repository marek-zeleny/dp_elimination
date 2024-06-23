#pragma once

#include <array>
#include <string>
#include "metrics/metrics_collector.hpp"
#include "utils.hpp"

namespace dp {

// counters
enum class MetricsCounters : uint8_t {
    MinVar = 0,
    MaxVar,
    InitVars,
    FinalVars,
    EliminatedVars,
    RemoveAbsorbedClausesCallCount,
    AbsorbedClausesRemoved,
    AbsorbedClausesNotAdded,
    UnitLiteralsRemoved,
    WatchedLiterals_Assignments,
    Last = WatchedLiterals_Assignments,
};

inline const std::array<std::string, to_underlying(MetricsCounters::Last) + 1> counter_names{
    "MinVar",
    "MaxVar",
    "InitVars",
    "FinalVars",
    "EliminatedVars",
    "RemoveAbsorbedClausesCallCount",
    "AbsorbedClausesRemoved",
    "AbsorbedClausesNotAdded",
    "UnitLiteralsRemoved",
    "WatchedLiterals_Assignments",
};

// series
enum class MetricsSeries : uint8_t {
    EliminatedLiterals = 0,
    ClauseCounts,
    NodeCounts,
    HeuristicScores,
    ClauseCountDifference,
    AbsorbedClausesRemoved,
    AbsorbedClausesNotAdded,
    UnitLiteralsRemoved,
    Last = UnitLiteralsRemoved,
};

inline const std::array<std::string, to_underlying(MetricsSeries::Last) + 1> series_names{
        "EliminatedLiterals",
        "ClauseCounts",
        "NodeCounts",
        "HeuristicScores",
        "ClauseCountDifference",
        "AbsorbedClausesRemoved",
        "AbsorbedClausesNotAdded",
        "UnitLiteralsRemoved",
};

// durations
enum class MetricsDurations : uint8_t {
    ReadInputFormula = 0,
    WriteOutputFormula,
    ReadFormula_AddClause,
    WriteFormula_PrintClause,
    AlgorithmTotal,
    RemoveAbsorbedClauses_Serialize,
    RemoveAbsorbedClauses_Search,
    RemoveAbsorbedClauses_Build,
    IncrementalAbsorbedRemoval_Serialize,
    IncrementalAbsorbedRemoval_Search,
    IncrementalAbsorbedRemoval_Build,
    VarSelection,
    EliminateVar_Total,
    EliminateVar_SubsetDecomposition,
    EliminateVar_Resolution,
    EliminateVar_TautologiesRemoval,
    EliminateVar_Unification,
    Last = EliminateVar_Unification,
};

inline const std::array<std::string, to_underlying(MetricsDurations::Last) + 1> duration_names{
    "ReadInputFormula",
    "WriteOutputFormula",
    "ReadFormula_AddClause",
    "WriteFormula_PrintClause",
    "AlgorithmTotal",
    "RemoveAbsorbedClauses_Serialize",
    "RemoveAbsorbedClauses_Search",
    "RemoveAbsorbedClauses_Build",
    "IncrementalAbsorbedRemoval_Serialize",
    "IncrementalAbsorbedRemoval_Search",
    "IncrementalAbsorbedRemoval_Build",
    "VarSelection",
    "EliminateVar_Total",
    "EliminateVar_SubsetDecomposition",
    "EliminateVar_Resolution",
    "EliminateVar_TautologiesRemoval",
    "EliminateVar_Unification",
};

// cumulative durations
enum class MetricsCumulativeDurations : uint8_t {
    WatchedLiterals_Propagation = 0,
    WatchedLiterals_Backtrack,
    Last = WatchedLiterals_Backtrack,
};

inline const std::array<std::string, to_underlying(MetricsCumulativeDurations::Last) + 1> cumulative_duration_names{
    "WatchedLiterals_Propagation",
    "WatchedLiterals_Backtrack",
};

inline MetricsCollector<MetricsCounters, MetricsSeries, MetricsDurations, MetricsCumulativeDurations> metrics{
    counter_names,
    series_names,
    duration_names,
    cumulative_duration_names
};

} // namespace dp
