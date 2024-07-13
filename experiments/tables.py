import pandas as pd
from typing import Callable

# data processing
elimination_table_keys: list[str] = [
    "Total",
    "SubsetDecomposition",
    "Resolution",
    "Unification",
    "TautologiesRemoval",
]


def create_elimination_table(metrics: dict) -> pd.DataFrame:
    data = [metrics["durations"]["EliminateVar_" + key] for key in elimination_table_keys]
    table = pd.DataFrame(data).transpose()
    table.columns = elimination_table_keys
    table["MeasurementOverhead"] = table["Total"] - table[elimination_table_keys[1:]].sum(axis=1)
    if table.empty:
        table.loc[0] = -1
    return table


def create_variables_table(metrics: dict) -> pd.DataFrame:
    data = metrics["durations"]["VarSelection"]
    table = pd.DataFrame(data)
    table.columns = ["VarSelection"]
    if table.empty:
        table.loc[0] = -1
    return table


def create_absorbed_table(metrics: dict) -> pd.DataFrame:
    if len(metrics["durations"]["RemoveAbsorbedClauses_Serialize"]) == 0:
        assert len(metrics["durations"]["RemoveAbsorbedClauses_Build"]) == 0
        data = [
            metrics["durations"]["RemoveAbsorbedClauses_Search"],
            metrics["series"]["AbsorbedClausesRemoved"],
        ]
        columns = ["Search", "AbsorbedClausesRemoved"]
    else:
        data = [
            metrics["durations"]["RemoveAbsorbedClauses_Serialize"],
            metrics["durations"]["RemoveAbsorbedClauses_Search"],
            metrics["durations"]["RemoveAbsorbedClauses_Build"],
            metrics["series"]["AbsorbedClausesRemoved"],
        ]
        columns = ["Serialize", "Search", "Build", "AbsorbedClausesRemoved"]

    table = pd.DataFrame(data).transpose()
    table.columns = columns
    table.insert(0, "TotalDuration", table.iloc[:, :-1].sum(axis=1))
    if table.empty:
        table.loc[0] = -1
    return table


def create_incremental_absorbed_table(metrics: dict) -> pd.DataFrame:
    if len(metrics["durations"]["IncrementalAbsorbedRemoval_Serialize"]) == 0:
        assert len(metrics["durations"]["IncrementalAbsorbedRemoval_Build"]) == 0
        data = [
            metrics["durations"]["IncrementalAbsorbedRemoval_Search"],
            metrics["series"]["AbsorbedClausesNotAdded"],
        ]
        columns = ["Search", "AbsorbedClausesNotAdded"]
    else:
        data = [
            metrics["durations"]["IncrementalAbsorbedRemoval_Serialize"],
            metrics["durations"]["IncrementalAbsorbedRemoval_Search"],
            metrics["durations"]["IncrementalAbsorbedRemoval_Build"],
            metrics["series"]["AbsorbedClausesNotAdded"],
        ]
        columns = ["Serialize", "Search", "Build", "AbsorbedClausesNotAdded"]

    table = pd.DataFrame(data).transpose()
    table.columns = columns
    table.insert(0, "TotalDuration", table.iloc[:, :-1].sum(axis=1))
    if table.empty:
        table.loc[0] = -1
    return table


def create_zbdd_size_table(metrics: dict) -> pd.DataFrame:
    data = [
        metrics["series"]["ClauseCounts"],
        metrics["series"]["NodeCounts"],
    ]
    table = pd.DataFrame(data).transpose()
    table.columns = ["ClauseCounts", "NodeCounts"]
    return table


def get_heuristic_correlation(metrics: dict) -> float:
    data = [
        metrics["series"]["HeuristicScores"],
        metrics["series"]["ClauseCountDifference"],
    ]
    table = pd.DataFrame(data).transpose()
    if table.empty:
        return 0
    correlation = table[0].corr(table[1])
    return correlation


def get_unit_propagations_per_second(metrics: dict) -> float:
    assignments = metrics["counters"]["WatchedLiterals_Assignments"]
    duration = metrics["cumulative_durations"]["WatchedLiterals_Propagation"]
    if duration == 0:
        return -1
    seconds = duration / 1_000_000
    return assignments / seconds


def get_backtrack_to_propagation_ratio(metrics: dict) -> float:
    backtrack = metrics["cumulative_durations"]["WatchedLiterals_Backtrack"]
    propagation = metrics["cumulative_durations"]["WatchedLiterals_Propagation"]
    if propagation == 0:
        return -1
    return backtrack / propagation


def create_overall_summary_table(metrics: dict, stages_summary: pd.DataFrame) -> pd.DataFrame:
    data = {
        "InitVars": metrics["counters"]["InitVars"],
        "FinalVars": metrics["counters"]["FinalVars"],
        "EliminatedVars": metrics["counters"]["EliminatedVars"],
        "InitClauses": metrics["series"]["ClauseCounts"][0],
        "FinalClauses": metrics["series"]["ClauseCounts"][-1],
        "RemovedUnitLiterals": metrics["counters"]["UnitLiteralsRemoved"],
        "RemovedAbsorbedClauses": metrics["counters"]["AbsorbedClausesRemoved"],
        "HeuristicCorrelation": get_heuristic_correlation(metrics),
        "ReadDuration": metrics["durations"]["ReadInputFormula"],
        "WriteDuration": metrics["durations"]["WriteOutputFormula"] or 0,
        "AlgorithmDuration": metrics["durations"]["AlgorithmTotal"],
        "VarSelection": stages_summary["VarSelection"].loc["sum"],
        "Elimination": stages_summary["Elimination"].loc["sum"],
        "AbsorbedRemoval": stages_summary["AbsorbedRemovalDuration"].loc["sum"],
        "UnitPropagationsPerSecond": get_unit_propagations_per_second(metrics),
        "BacktrackToPropagationRatio": get_backtrack_to_propagation_ratio(metrics),
    }
    table = pd.DataFrame(data)
    return table


def get_part_of_total_func(total_sum: int, multiplier: float) -> Callable[[pd.Series], float]:
    def ret_0(df_col: pd.Series) -> float:
        assert df_col.sum() == 0
        return 0
    def part_of_total(df_col: pd.Series) -> float:
        return (df_col.sum() / total_sum) * multiplier
    if total_sum == 0:
        return ret_0
    else:
        return part_of_total


def std(df_col: pd.Series) -> float:
    if len(df_col) < 2:
        return 0
    else:
        return df_col.std()


def rel_std(df_col: pd.Series) -> float:
    mean = df_col.mean()
    if mean == 0:
        return std(df_col)
    else:
        return (std(df_col) / mean) * 100


def get_aggregation_functions(total_sum: int) -> list:
    return [
        "count",
        "sum",
        get_part_of_total_func(total_sum, 100_000),
        "mean",
        "median",
        std,
        rel_std,
        "max",
        "argmax",
    ]


def create_tables(metrics: dict) -> list[tuple[str, pd.DataFrame, bool]]:
    tables = []

    elimination = create_elimination_table(metrics)

    total_sum = elimination["Total"].sum()
    aggregation_functions = get_aggregation_functions(total_sum)

    elimination_summary = elimination.aggregate(aggregation_functions).astype(int, copy=False)
    tables.append(("elimination_stages", elimination_summary.drop("count"), True))

    variables = create_variables_table(metrics)
    variables_summary = variables.aggregate(aggregation_functions).astype(int, copy=False)

    absorbed = create_absorbed_table(metrics)
    absorbed_summary = absorbed.aggregate(aggregation_functions).astype(int, copy=False)
    incremental_absorbed = create_incremental_absorbed_table(metrics)
    incremental_summary = incremental_absorbed.aggregate(aggregation_functions).astype(int, copy=False)
    absorbed_and_incremental_absorbed_summary = absorbed_summary.add_prefix("Removed_").join(incremental_summary.add_prefix("Incremental_"))
    tables.append(("absorbed_clauses", absorbed_and_incremental_absorbed_summary, True))

    stages_summary = pd.concat([
        elimination_summary[["Total"]].rename(columns={"Total": "Elimination"}),
        variables_summary,
        absorbed_summary[["TotalDuration", "AbsorbedClausesRemoved"]].rename(columns={"TotalDuration": "AbsorbedRemovalDuration"})
    ], axis=1).rename(index={"part_of_total": "ratio"})
    stages_summary.loc["ratio"] //= 100
    tables.append(("algorithm_stages", stages_summary, True))

    sizes = create_zbdd_size_table(metrics)
    sizes_summary = sizes.aggregate(aggregation_functions[3:]).astype(int, copy=False)
    tables.append(("zbdd_sizes", sizes_summary, True))

    overall_summary = create_overall_summary_table(metrics, stages_summary)
    tables.append(("summary", overall_summary, False))
    return tables
