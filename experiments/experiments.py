#!/usr/bin/env python3

import os
import sys
import subprocess
import time
import math
import argparse
import json
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import ticker
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import timedelta
from typing import Callable, Generator

# path constants
script_root_dir: Path = Path(os.path.realpath(__file__)).parent.absolute()
default_config_path: Path = script_root_dir / "default_config.toml"
inputs_dir: Path = script_root_dir / "inputs"
setups_dir: Path = script_root_dir / "setups"

# experiments
input_formulas: list[str] = [
    "dpelim_data/Handmade/ais/ais6.dc",
    "dpelim_data/Handmade/ais/ais8.dc",
    "dpelim_data/Handmade/ais/ais10.dc",
    "dpelim_data/Handmade/ais/ais12.dc",
    "dpelim_data/Planning/3blocks.dc",
    "dpelim_data/Planning/tire-1.dc",
    "dpelim_data/Planning/tire-2.dc",
    "dpelim_data/Planning/tire-3.dc",
    "dpelim_data/Planning/tire-4.dc",
    "dpelim_data/Network/sat-grid-pbl-0010.dc",
    "dpelim_data/Network/sat-grid-pbl-0015.dc",
    "dpelim_data/Network/Ace/mastermind_03_08_04.net.dc",
    "dpelim_data/Network/Ace/mastermind_05_08_03.net.dc",
    "dpelim_data/Network/Ace/mastermind_06_08_03.net.dc",
]

experiment_setups: list[str] = [
    "minimal_bloat",
]


def generate_setups(results_dir: Path) -> Generator[tuple[Path, Path, Path, Path], None, None]:
    for formula in input_formulas:
        for setup in experiment_setups:
            setup_config_path = setups_dir / f"{setup}.toml"
            input_config_path = inputs_dir / f"{formula}.toml"
            input_formula_path = inputs_dir / f"{formula}.cnf"
            output_dir_path = results_dir / f"{setup}__{formula}"
            yield setup_config_path, input_config_path, input_formula_path, output_dir_path


def count_setups(args):
    print(len(list(generate_setups(Path()))))


def run_experiment(command: list[str], cwd: Path, is_run_in_parallel: bool = True) -> int:
    if is_run_in_parallel:
        out_file = open(cwd / "out.txt", "w")
        err_file = out_file
    else:
        out_file = sys.stdout
        err_file = sys.stderr

    def print_out(*args, **kwargs):
        print(*args, **kwargs, file=out_file)

    def print_err(*args, **kwargs):
        print(*args, **kwargs, file=err_file)

    print_out(" ".join(command))
    print_out()
    print_out("output:", flush=True)

    start_time = time.monotonic()
    result: subprocess.CompletedProcess[str] = subprocess.run(command, cwd=cwd, stdout=out_file, stderr=err_file)
    end_time = time.monotonic()
    duration = timedelta(seconds=end_time - start_time)

    print_out()
    print_out(f"Command exited with code {result.returncode}")

    block = "=" * 10
    if result.returncode == 0:
        result_msg = "Experiment finished"
    else:
        result_msg = "Experiment failed"
    print_out(f"{block} {result_msg}, runtime {duration} {block}", flush=True)

    if out_file is not sys.stdout:
        out_file.flush()
        out_file.close()
    return result.returncode


def run_dp_experiments(args):
    dp_path = Path(args.dp_executable)
    if not dp_path.exists():
        print(f"Invalid path to dp executable: {dp_path}", file=sys.stderr)
        sys.exit(1)
    results_dir = Path(args.results_dir).absolute()
    # prepare execution setups
    setup_index = args.setup_index
    commands: list[list[str]] = []
    working_dirs: list[Path] = []
    for i, (setup_config_path,
            input_config_path,
            input_formula_path,
            output_dir_path) in enumerate(generate_setups(results_dir)):
        if setup_index >= 0 and i != setup_index:
            continue
        command_with_args = [str(dp_path.absolute()),
                             str(input_formula_path),
                             "--config", str(default_config_path),
                             "--config", str(setup_config_path),
        ]
        if input_config_path.exists():
            command_with_args += ["--config", str(input_config_path)]
        os.makedirs(output_dir_path, exist_ok=True)
        commands.append(command_with_args)
        working_dirs.append(output_dir_path)
    # execute in parallel
    num_processes = args.processes
    if num_processes == 1:
        in_parallel = [False for _ in range(len(commands))]
        print(f"Running {len(commands)} experiments serially\n")
    else:
        assert num_processes > 1
        assert len(commands) > num_processes
        in_parallel = [True for _ in range(len(commands))]
        print(f"Running {len(commands)} experiments in parallel (max {num_processes} processes)\n")
    with ThreadPoolExecutor(max_workers=num_processes) as exec:
        exit_codes = exec.map(run_experiment, commands, working_dirs, in_parallel)
    # print results summary
    print()
    for dir, code in zip(working_dirs, exit_codes):
        print(f"Experiment {dir.name} exited with code {code}")


# data processing template
def process_metrics(results_dir_path: str, func: Callable[[Path, dict], None]):
    results_dir = Path(results_dir_path).absolute()
    if not results_dir.exists() or not results_dir.is_dir():
        print(f"Invalid path to results: {results_dir}", file=sys.stderr)
        sys.exit(1)
    for _, _, _, output_dir_path in generate_setups(results_dir):
        metrics_path = output_dir_path / "metrics.json"
        if metrics_path.exists() and metrics_path.is_file():
            print(f"Processing metrics file {metrics_path}", flush=True)
            with open(metrics_path, "r") as file:
                metrics: dict = json.load(file)
                func(output_dir_path, metrics)
        else:
            print(f"Metrics file {metrics_path} doesn't exist", file=sys.stderr, flush=True)

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
    seconds = duration / 1_000_000
    return assignments / seconds


def get_backtrack_to_propagation_ratio(metrics: dict) -> float:
    backtrack = metrics["cumulative_durations"]["WatchedLiterals_Backtrack"]
    propagation = metrics["cumulative_durations"]["WatchedLiterals_Propagation"]
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
        "WriteDuration": metrics["durations"]["WriteOutputFormula"],
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


def summarize_metrics(args):
    results_dir = args.results_dir
    format = args.format
    def process(output_dir_path: Path, metrics: dict):
        tables = create_tables(metrics)
        for name, table, include_index in tables:
            export_table(table, output_dir_path / f"{name}.{format}", format, include_index)
    process_metrics(results_dir, process)

# plotting
scaling_factor_units_map = {
    0: "us",
    1: "ms",
    2: "s",
    3: "10^3 s",
}


def get_divider(factor: int):
    def divider(val: int, *args):
        return int(val // factor)
    return divider


def get_axes_scaling_factor(ax: plt.Axes) -> tuple[int, str]:
    min_value, max_value = ax.get_ylim()
    extreme = max(abs(min_value), abs(max_value))
    if extreme < 1000:
        exp = 0
    else:
        exp = math.floor(math.log(extreme, 1000))
        if extreme / (1000 ** exp) < 4:
            exp -= 1
    factor = 1000 ** exp
    unit = scaling_factor_units_map[exp]
    return int(factor), unit


def plot_zbdd_size(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["ClauseCounts"], "orange", label="clauses")
    ax.plot(series["NodeCounts"], "red", label="nodes")

    ax.set_title("ZBDD size")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("count")
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_heuristic_accuracy(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["HeuristicScores"], "blue", label="heuristic score")
    ax.plot(series["ClauseCountDifference"], "orange", label="clause count difference")

    ax.set_title("Heuristic accuracy")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("(expected) # clauses")
    ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_unit_literals(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["UnitLiteralsRemoved"], "red", label="unit literals")

    ax.set_title("Unit literals removed")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("# literals")
    ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def plot_absorbed_clauses(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    absorbed_removed = metrics["series"]["AbsorbedClausesRemoved"]
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    xticklabels = range(1, len(absorbed_removed) + 1)
    ax.set_title("Removed absorbed clauses")
    ax.set_xlabel("# invocations")
    ax.set_ylabel("# absorbed clauses removed")
    ax.bar(xticklabels, absorbed_removed, color="coral", width=0.4, label="removed clauses")
    ax.set_ylim(bottom=0)

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    ax.plot(durations["RemoveAbsorbedClauses_Search"], "green", label="search")
    if len(durations["RemoveAbsorbedClauses_Serialize"]) > 0:
        assert len(durations["RemoveAbsorbedClauses_Build"]) > 0
        ax.plot(durations["RemoveAbsorbedClauses_Serialize"], "blue", label="serialize")
        ax.plot(durations["RemoveAbsorbedClauses_Build"], "cyan", label="build")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_incremental_absorbed(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    absorbed_not_added = metrics["series"]["AbsorbedClausesNotAdded"]
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    xticklabels = range(1, len(absorbed_not_added) + 1)
    ax.set_title("Incremental absorbed clause removal")
    ax.set_xlabel("# invocations")
    ax.set_ylabel("# absorbed clauses not added")
    ax.bar(xticklabels, absorbed_not_added, color="coral", width=0.4, label="absorbed clauses")
    ax.set_ylim(bottom=0)

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    ax.plot(durations["IncrementalAbsorbedRemoval_Search"], "green", label="search")
    if len(durations["IncrementalAbsorbedRemoval_Serialize"]) > 0:
        assert len(durations["IncrementalAbsorbedRemoval_Build"]) > 0
        ax.plot(durations["IncrementalAbsorbedRemoval_Serialize"], "blue", label="serialize")
        ax.plot(durations["IncrementalAbsorbedRemoval_Build"], "cyan", label="build")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_elimination_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["EliminateVar_SubsetDecomposition"], "orange", label="subset decomposition")
    ax.plot(durations["EliminateVar_Resolution"], "brown", label="resolution")
    ax.plot(durations["EliminateVar_TautologiesRemoval"], "royalblue", label="tautologies removal")
    ax.plot(durations["EliminateVar_Unification"], "green", label="unification")
    ax.plot(durations["EliminateVar_Total"], "red", label="total")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of variable elimination")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.25)
    return fig, axes


def plot_read_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["ReadFormula_AddClause"], "green", label="clause addition")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of reading input clause")
    ax.set_xlabel("# added clauses")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def plot_write_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["WriteFormula_PrintClause"], "orange", label="clause writing")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of writing output clause")
    ax.set_xlabel("# written clauses")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def create_plots(metrics: dict) -> list[tuple[str, plt.Figure]]:
    figures = []

    fig_zbdd_size, _ = plot_zbdd_size(metrics)
    figures.append(("zbdd_size", fig_zbdd_size))

    fig_eliminated, _ = plot_heuristic_accuracy(metrics)
    figures.append(("heuristic", fig_eliminated))

    fig_unit_literals, _ = plot_unit_literals(metrics)
    figures.append(("unit_literals", fig_unit_literals))

    fig_absorbed, _ = plot_absorbed_clauses(metrics)
    figures.append(("absorbed", fig_absorbed))

    fig_incremental_absorbed, _ = plot_incremental_absorbed(metrics)
    figures.append(("incremental_absorbed", fig_incremental_absorbed))

    fig_elimination_duration, _ = plot_elimination_duration(metrics)
    figures.append(("duration_elimination", fig_elimination_duration))

    fig_read_duration, _ = plot_read_duration(metrics)
    figures.append(("duration_read", fig_read_duration))

    fig_write_duration, _ = plot_write_duration(metrics)
    figures.append(("duration_write", fig_write_duration))

    return figures


def export_table(table: pd.DataFrame, path: Path, format: str, include_index: bool):
    if format == "md":
        if len(table.columns) > 8:
            output = table.iloc[:, :8].astype(str).to_markdown(index=include_index, tablefmt="pretty") + "\n" + \
                     table.iloc[:, 8:].astype(str).to_markdown(index=include_index, tablefmt="pretty")
        else:
            output = table.astype(str).to_markdown(index=include_index, tablefmt="pretty")
    elif format == "tex":
        output = table.to_latex(index=include_index)
    else:
        raise NotImplementedError(f"Table format {format} not supported")

    with open(path, "w") as file:
        file.write(output)


def visualize_metrics(args):
    results_dir = args.results_dir
    format = args.format
    dpi = args.dpi
    def process(output_dir_path: Path, metrics: dict):
        plots = create_plots(metrics)
        for name, fig in plots:
            fig.savefig(output_dir_path / f"{name}.{format}", format=format, dpi=dpi)
            plt.close(fig)
    process_metrics(results_dir, process)


# CLI
parser = argparse.ArgumentParser(description="Run DP experiments and visualize results",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
subparsers = parser.add_subparsers()

parser_count = subparsers.add_parser("count",
                                     description="Count the number of experiment setups that will be run",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_count.set_defaults(func=count_setups)

parser_run = subparsers.add_parser("run",
                                   description="Run experiments",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_run.set_defaults(func=run_dp_experiments)
parser_run.add_argument("dp_executable", type=str, help="Path to the compiled DP executable")
parser_run.add_argument("-r", "--results-dir", type=str, default="results", help="Directory for storing results")
parser_run.add_argument("-p", "--processes", type=int, default=1, help="Number of processes spawned concurrently")
parser_run.add_argument("-i", "--setup-index",
                        type=int,
                        default=-1,
                        help="Run only one experimental setup at the given index")

parser_summary = subparsers.add_parser("summarize",
                                       description="Process metrics from experiments and create summary tables",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_summary.set_defaults(func=summarize_metrics)
parser_summary.add_argument("results_dir",
                            type=str,
                            help="Directory with results (given as '--results-dir' when running experiments)")
parser_summary.add_argument("-f", "--format", type=str, default="md", help="Format of exported tables")

parser_visualize = subparsers.add_parser("visualize",
                                         description="Process metrics from experiments and visualize them",
                                         formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_visualize.set_defaults(func=visualize_metrics)
parser_visualize.add_argument("results_dir",
                              type=str,
                              help="Directory with results (given as '--results-dir' when running experiments)")
parser_visualize.add_argument("-f", "--format", type=str, default="png", help="Format of plot files")
parser_visualize.add_argument("-r", "--dpi", "--resolution", type=int, default=150, help="Resolution of plots")

if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
