#pragma once

#include <array>
#include <string>
#include "metrics/metrics_collector.hpp"
#include "utils.hpp"

namespace dp {

// counters
enum class MetricsCounters : uint8_t {
    TotalVars = 0,
    TotalClauses,
    EliminatedVars,
    RemovedClauses,
    RemoveAbsorbedClausesCallCount,
    AbsorbedClausesRemovedTotal,
    Last = AbsorbedClausesRemovedTotal,
};

inline const std::array<std::string, to_underlying(MetricsCounters::Last) + 1> counter_names{
    "TotalVars",
    "TotalClauses",
    "EliminatedVars",
    "RemovedClauses",
    "RemoveAbsorbedClausesCallCount",
    "AbsorbedClausesRemovedTotal",
};

// series
enum class MetricsSeries : uint8_t {
    EliminatedLiterals = 0,
    ClauseCounts,
    NodeCounts,
    HeuristicScores,
    EliminatedClauses,
    AbsorbedClausesRemoved,
    Last = AbsorbedClausesRemoved,
};

inline const std::array<std::string, to_underlying(MetricsSeries::Last) + 1> series_names{
        "EliminatedLiterals",
        "ClauseCounts",
        "NodeCounts",
        "HeuristicScores",
        "EliminatedClauses",
        "AbsorbedClausesRemoved",
};

// durations
enum class MetricsDurations : uint8_t {
    ReadInputFormula = 0,
    WriteOutputFormula,
    ReadFormula_AddClause,
    WriteFormula_PrintClause,
    EliminateVars,
    RemoveAbsorbedClausesWithConversion,
    VarSelection,
    EliminateVar_Total,
    EliminateVar_SubsetDecomposition,
    EliminateVar_Resolution,
    EliminateVar_TautologiesRemoval,
    EliminateVar_SubsumedRemoval1,
    EliminateVar_SubsumedRemoval2,
    EliminateVar_Unification,
    Last = EliminateVar_Unification,
};

inline const std::array<std::string, to_underlying(MetricsDurations::Last) + 1> duration_names{
    "ReadInputFormula",
    "WriteOutputFormula",
    "ReadFormula_AddClause",
    "WriteFormula_PrintClause",
    "EliminateVars",
    "RemoveAbsorbedClausesWithConversion",
    "VarSelection",
    "EliminateVar_Total",
    "EliminateVar_SubsetDecomposition",
    "EliminateVar_Resolution",
    "EliminateVar_TautologiesRemoval",
    "EliminateVar_SubsumedRemoval1",
    "EliminateVar_SubsumedRemoval2",
    "EliminateVar_Unification",
};

inline MetricsCollector<MetricsCounters, MetricsSeries, MetricsDurations> metrics{counter_names,
                                                                                  series_names,
                                                                                  duration_names};

} // namespace dp
