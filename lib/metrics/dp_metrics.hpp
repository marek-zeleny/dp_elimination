#pragma once

#include <array>
#include <string>
#include "metrics/metrics_collector.hpp"
#include "utils.hpp"

namespace dp {

// counters
enum class MetricsCounters : uint8_t {
    RemoveAbsorbedClausesCallCount = 0,
    AbsorbedClausesRemovedTotal = 1,
    Last = AbsorbedClausesRemovedTotal,
};

inline const std::array<std::string, to_underlying(MetricsCounters::Last) + 1> counter_names{
    "RemoveAbsorbedClausesCallCount",
    "AbsorbedClausesRemovedTotal",
};

// series
enum class MetricsSeries : uint8_t {
    EliminatedLiterals = 0,
    AbsorbedClausesRemoved = 1,
    Last = AbsorbedClausesRemoved,
};

inline const std::array<std::string, to_underlying(MetricsSeries::Last) + 1> series_names{
        "EliminatedLiterals",
        "AbsorbedClausesRemoved",
};

// durations
enum class MetricsDurations : uint8_t {
    EliminateVars = 0,
    RemoveAbsorbedClausesWithConversion = 1,
    EliminateVar_Total = 2,
    EliminateVar_SubsetDecomposition = 3,
    EliminateVar_Resolution = 4,
    EliminateVar_TautologiesRemoval = 5,
    EliminateVar_SubsumedRemoval1 = 6,
    EliminateVar_SubsumedRemoval2 = 7,
    EliminateVar_Unification = 8,
    Last = EliminateVar_Unification,
};

inline const std::array<std::string, to_underlying(MetricsDurations::Last) + 1> duration_names{
    "EliminateVars",
    "RemoveAbsorbedClausesWithConversion",
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
