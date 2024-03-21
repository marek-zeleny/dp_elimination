#pragma once

#include "metrics/metrics_collector.hpp"

namespace dp {

enum class MetricsCounters : uint16_t {
    RemoveAbsorbedClausesCallCount = 0,
    AbsorbedClausesRemovedTotal = 1,
    Last = AbsorbedClausesRemovedTotal,
};

enum class MetricsDurations : uint16_t {
    EliminateVars = 0,
    RemoveAbsorbedClausesWithConversion = 1,
    EliminateVar_Total = 2,
    EliminateVar_SubsetDecomposition = 3,
    EliminateVar_Resolution = 4,
    EliminateVar_TautologiesRemoval = 5,
    EliminateVar_SubsumedRemoval = 6,
    EliminateVar_Unification = 7,
    Last = EliminateVar_Unification,
};

inline MetricsCollector<MetricsCounters, MetricsDurations> metrics;

} // namespace dp
