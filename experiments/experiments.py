import os
import sys
import subprocess
import argparse
import json
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import ticker
from pathlib import Path
from typing import Callable, Generator

# path constants
script_root_dir: Path = Path(os.path.realpath(__file__)).parent.absolute()
default_config_path: Path = script_root_dir / "default_config.toml"
inputs_dir: Path = script_root_dir / "inputs"
setups_dir: Path = script_root_dir / "setups"

# experiments
input_formulas: list[str] = [
    "bf0432-007",
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


def run_dp_experiments(args):
    dp_path = Path(args.dp_executable)
    if not dp_path.exists():
        print(f"Invalid path to dp executable: {dp_path}")
        sys.exit(1)
    results_dir = Path(args.results_dir).absolute()
    for setup_config_path, input_config_path, input_formula_path, output_dir_path in generate_setups(results_dir):
        command_with_args = [str(dp_path.absolute()),
                             "--config", str(default_config_path),
                             "--config", str(setup_config_path),
                             "--config", str(input_config_path),
                             "--input-file", str(input_formula_path),
        ]
        os.makedirs(output_dir_path, exist_ok=True)
        print(" ".join(command_with_args))
        print()
        subprocess.run(command_with_args, cwd=output_dir_path)
        block = "=" * 10
        print(f"{block} Experiment finished {block}")

# data processing
elimination_table_keys: list[str] = [
    "Total",
    "SubsetDecomposition",
    "Resolution",
    "Unification",
    "SubsumedRemoval1",
    "SubsumedRemoval2",
    "TautologiesRemoval",
]


def create_elimination_table(metrics: dict) -> pd.DataFrame:
    data = [metrics["durations"]["EliminateVar_" + key] for key in elimination_table_keys]
    table = pd.DataFrame(data).transpose()
    table.columns = elimination_table_keys
    table["MeasurementOverhead"] = table["Total"] - table[elimination_table_keys[1:]].sum(axis=1)
    return table


def create_variables_table(metrics: dict) -> pd.DataFrame:
    data = metrics["durations"]["VarSelection"]
    table = pd.DataFrame(data)
    table.columns = ["VarSelection"]
    return table


def create_absorbed_table(metrics: dict) -> pd.DataFrame:
    data = [
        metrics["durations"]["RemoveAbsorbedClausesWithConversion"],
        metrics["series"]["AbsorbedClausesRemoved"],
    ]
    table = pd.DataFrame(data).transpose()
    table.columns = ["AbsorbedRemovalDuration", "AbsorbedClausesRemoved"]
    return table


def get_heuristic_correlation(metrics: dict) -> float:
    data = [
        metrics["series"]["HeuristicScores"],
        metrics["series"]["EliminatedClauses"],
    ]
    table = pd.DataFrame(data).transpose()
    correlation = table[0].corr(table[1])
    return correlation


def create_overall_summary_table(metrics: dict, stages_summary: pd.DataFrame) -> pd.DataFrame:
    data = {
        "TotalVars": metrics["counters"]["TotalVars"],
        "EliminatedVars": metrics["counters"]["EliminatedVars"],
        "TotalClauses": metrics["counters"]["TotalClauses"],
        "RemovedClauses": metrics["counters"]["RemovedClauses"],
        "RemovedAbsorbedClauses": metrics["counters"]["AbsorbedClausesRemovedTotal"],
        "HeuristicCorrelation": get_heuristic_correlation(metrics),
        "TotalDuration": metrics["durations"]["EliminateVars"],
        "VarSelection": stages_summary["VarSelection"].loc["sum"],
        "Elimination": stages_summary["Elimination"].loc["sum"],
        "AbsorbedRemoval": stages_summary["AbsorbedRemovalDuration"].loc["sum"],
    }
    table = pd.DataFrame(data)
    return table


def get_part_of_total_func(total_sum: int, multiplier: float) -> Callable[[pd.Series], float]:
    def part_of_total(df_col: pd.Series) -> float:
        return (df_col.sum() / total_sum) * multiplier
    return part_of_total


def rel_std(df_col: pd.Series) -> float:
    return (df_col.std() / df_col.mean()) * 100


def get_aggregation_functions(total_sum: int) -> list:
    return [
        "count",
        "sum",
        get_part_of_total_func(total_sum, 100_000),
        "mean",
        "median",
        "std",
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

    stages_summary = pd.concat([elimination_summary["Total"], variables_summary, absorbed_summary], axis=1) \
                    .rename(columns={"Total": "Elimination"}, index={"part_of_total": "ratio"})
    stages_summary.loc["ratio"] //= 100
    tables.append(("algorithm_stages", stages_summary, True))

    overall_summary = create_overall_summary_table(metrics, stages_summary)
    tables.append(("summary", overall_summary, False))
    return tables


# plotting
def get_divider(factor: int):
    def divider(val: int, *args):
        return int(val // factor)
    return divider


def plot_eliminated_clauses(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.set_title("Eliminated clauses")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("inverted heuristic score")
    ax.plot([-x for x in series["HeuristicScores"]], "blue", label="heuristic")

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    ax.set_ylabel("# eliminated clauses")
    ax.plot(series["EliminatedClauses"], "orange", label="clauses")

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_absorbed_clauses(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    absorbed_removed = metrics["series"]["AbsorbedClausesRemoved"]
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.set_title("Removed absorbed clauses")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("duration (ms)")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(1000)))
    ax.plot(durations["RemoveAbsorbedClausesWithConversion"], "green", label="duration")

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    xticklabels = [str(10 * x) for x in range(len(absorbed_removed))]
    ax.set_ylabel("# absorbed clauses removed")
    ax.set_yscale("log")
    ax.bar(xticklabels, absorbed_removed, width=0.8, label="removed absorbed clauses")
    ax.xaxis.set_major_locator(ticker.MultipleLocator(5))

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_total_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.set_title("Total duration of elimination")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("duration (ms)")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(1000)))
    ax.plot(durations["EliminateVar_Total"], "red", label="elimination")
    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def plot_duration_detail(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.set_title("Durations of elimination stages")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("duration (ms)")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(1000)))
    ax.plot(durations["EliminateVar_SubsetDecomposition"], "orange", label="subset decomposition")
    ax.plot(durations["EliminateVar_Resolution"], "blue", label="resolution")
    ax.plot(durations["EliminateVar_TautologiesRemoval"], "cyan", label="tautologies removal")
    ax.plot(durations["EliminateVar_SubsumedRemoval1"], "magenta", label="subsumed removal (before unification)")
    ax.plot(durations["EliminateVar_SubsumedRemoval2"], "brown", label="subsumed removal (after unification)")
    ax.plot(durations["EliminateVar_Unification"], "green", label="unification")
    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.25)
    return fig, axes


def create_plots(metrics: dict) -> list[tuple[str, plt.Figure]]:
    figures = []

    fig_eliminated, _ = plot_eliminated_clauses(metrics)
    figures.append(("eliminated", fig_eliminated))

    fig_absorbed, _ = plot_absorbed_clauses(metrics)
    figures.append(("absorbed", fig_absorbed))

    fig_total_duration, _ = plot_total_duration(metrics)
    figures.append(("duration_total", fig_total_duration))

    fig_duration_detail, _ = plot_duration_detail(metrics)
    figures.append(("duration_detail", fig_duration_detail))

    return figures


def export_table(table: pd.DataFrame, path: Path, format: str, include_index: bool):
    if format == "md":
        output = table.astype(str).to_markdown(index=include_index, tablefmt="pretty")
    elif format == "tex":
        output = table.to_latex(index=include_index)
    else:
        raise NotImplementedError()

    with open(path, "w") as file:
        file.write(output)


def visualize_metrics(args):
    results_dir = Path(args.results_dir).absolute()
    if not results_dir.exists() or not results_dir.is_dir():
        print(f"Invalid path to results: {results_dir}")
        sys.exit(1)
    for _, _, _, output_dir_path in generate_setups(results_dir):
        metrics_path = output_dir_path / "metrics.json"
        with open(metrics_path, "r") as file:
            metrics: dict = json.load(file)
            tables = create_tables(metrics)
            for name, table, include_index in tables:
                format = args.table_format
                export_table(table, output_dir_path / f"{name}.{format}", format, include_index)
            plots = create_plots(metrics)
            for name, fig in plots:
                format = args.plot_format
                fig.savefig(output_dir_path / f"{name}.{format}", format=format, dpi=args.dpi)


# CLI
parser = argparse.ArgumentParser(description="Run DP experiments and visualize results")
subparsers = parser.add_subparsers()

parser_run = subparsers.add_parser("run", description="Run experiments")
parser_run.set_defaults(func=run_dp_experiments)
parser_run.add_argument("dp_executable", type=str, help="Path to the compiled DP executable")
parser_run.add_argument("-r", "--results-dir", type=str, default="results", help="Directory for storing results")

parser_visual = subparsers.add_parser("visual", description="Process metrics after experiments and visualize them")
parser_visual.add_argument("results_dir", type=str,
                           help="Directory with results (given as '--results-dir' when running experiments)")
parser_visual.add_argument("-r", "--dpi", "--resolution", type=int, default=150, help="Resolution of plots")
parser_visual.add_argument("-p", "--plot-format", type=str, default="png", help="Format of plot files")
parser_visual.add_argument("-t", "--table-format", type=str, default="md", help="Format of exported tables")
parser_visual.set_defaults(func=visualize_metrics)


if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
