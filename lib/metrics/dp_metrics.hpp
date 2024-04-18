#pragma once

#include <array>
#include <string>
#include "metrics/metrics_collector.hpp"
#include "utils.hpp"

namespace dp {

// counters
enum class MetricsCounters : uint8_t {
    InitVars = 0,
    FinalVars,
    EliminatedVars,
    RemoveAbsorbedClausesCallCount,
    AbsorbedClausesRemoved,
    UnitLiteralsRemoved,
    Last = UnitLiteralsRemoved,
};

inline const std::array<std::string, to_underlying(MetricsCounters::Last) + 1> counter_names{
    "InitVars",
    "FinalVars",
    "EliminatedVars",
    "RemoveAbsorbedClausesCallCount",
    "AbsorbedClausesRemoved",
    "UnitLiteralsRemoved",
};

// series
enum class MetricsSeries : uint8_t {
    EliminatedLiterals = 0,
    ClauseCounts,
    NodeCounts,
    HeuristicScores,
    ClauseCountDifference,
    AbsorbedClausesRemoved,
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
    "VarSelection",
    "EliminateVar_Total",
    "EliminateVar_SubsetDecomposition",
    "EliminateVar_Resolution",
    "EliminateVar_TautologiesRemoval",
    "EliminateVar_Unification",
};

inline MetricsCollector<MetricsCounters, MetricsSeries, MetricsDurations> metrics{counter_names,
                                                                                  series_names,
                                                                                  duration_names};

} // namespace dp
