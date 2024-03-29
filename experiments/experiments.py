import os
import sys
import subprocess
import argparse
import json
import matplotlib.pyplot as plt
from matplotlib import ticker
from pathlib import Path
from typing import Generator

script_root_dir: Path = Path(os.path.realpath(__file__)).parent.absolute()
default_config_path: Path = script_root_dir / "default_config.toml"
inputs_dir: Path = script_root_dir / "inputs"
setups_dir: Path = script_root_dir / "setups"

input_formulas: list[str] = [
    "bf0432-007",
]

experiment_setups: list[str] = [
    "minimal_bloat",
]


def get_divider(factor: int):
    def divider(val: int, *args):
        return int(val // factor)
    return divider


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


def create_summary_table(metrics: dict):
    ...


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


def visualize_metrics(args):
    results_dir = Path(args.results_dir).absolute()
    if not results_dir.exists() or not results_dir.is_dir():
        print(f"Invalid path to dp executable: {results_dir}")
        sys.exit(1)
    for _, _, _, output_dir_path in generate_setups(results_dir):
        metrics_path = output_dir_path / "metrics.json"
        with open(metrics_path, "r") as file:
            metrics: dict = json.load(file)
            summary = create_summary_table(metrics)
            plots = create_plots(metrics)
            for name, fig in plots:
                format = args.format
                fig.savefig(output_dir_path / f"{name}.{format}", format=format, dpi=args.dpi)


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
parser_visual.add_argument("-f", "--format", type=str, default="png", help="Format of plot files")
parser_visual.set_defaults(func=visualize_metrics)


if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
