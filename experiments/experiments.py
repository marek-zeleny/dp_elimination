#!/usr/bin/env python3

import os
import sys
import argparse
import json
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from typing import Callable, Generator
from run import run_dp_experiments
from tables import create_tables
from plots import create_plots
from summary import extract_setup_summary_data, create_setup_summary_table, create_setup_summary_plots

# path constants
script_root_dir: Path = Path(os.path.realpath(__file__)).parent.absolute()
input_formulas_list_path: Path = script_root_dir / "input_formulas.txt"
default_config_path: Path = script_root_dir / "default_config.toml"
inputs_dir: Path = script_root_dir / "inputs"
setups_dir: Path = script_root_dir / "setups"

# setups
input_formulas: list[str] = None
experiment_setups: list[str] = [
    "all_minimizations",
    "only_complete_minimization",
    "no_absorbed_removal",
]


def get_input_formulas() -> list[str]:
    global input_formulas
    if not input_formulas:
        with open(input_formulas_list_path, "r") as file:
            input_formulas = [line.rstrip() for line in file.readlines()]
    return input_formulas


def generate_setups(results_dir: Path) -> Generator[tuple[Path, Path, Path, Path], None, None]:
    for formula in get_input_formulas():
        for setup in experiment_setups:
            setup_config_path = setups_dir / f"{setup}.toml"
            input_config_path = inputs_dir / f"{formula}.toml"
            input_formula_path = inputs_dir / f"{formula}.cnf"
            output_dir_path = results_dir / f"{setup}/{formula}"
            yield setup, formula, setup_config_path, input_config_path, input_formula_path, output_dir_path


def count_setups(args):
    print(len(list(generate_setups(Path()))))


# execution
def run_experiments(args):
    dp_path = Path(args.dp_executable)
    results_dir = Path(args.results_dir)
    num_processes = args.processes
    setup_index = args.setup_index
    run_dp_experiments(dp_path, results_dir, num_processes, setup_index)


# data processing template
def process_metrics(results_dir_path: str, func: Callable[[Path, dict], None]):
    results_dir = Path(results_dir_path).absolute()
    if not results_dir.exists() or not results_dir.is_dir():
        print(f"Invalid path to results: {results_dir}", file=sys.stderr)
        sys.exit(1)
    for setup, formula, _, _, _, output_dir_path in generate_setups(results_dir):
        metrics_path = output_dir_path / "metrics.json"
        if metrics_path.exists() and metrics_path.is_file():
            print(f"Processing metrics file {metrics_path}", flush=True)
            with open(metrics_path, "r") as file:
                metrics: dict = json.load(file)
                func(output_dir_path, metrics, setup, formula)
        else:
            print(f"Metrics file {metrics_path} doesn't exist", file=sys.stderr, flush=True)


# data processing
def summarize_metrics(args):
    results_dir = args.results_dir
    format = args.format
    def process(output_dir_path: Path, metrics: dict, *_):
        tables = create_tables(metrics)
        for name, table, include_index in tables:
            export_table(table, output_dir_path / f"{name}.{format}", format, include_index)
    process_metrics(results_dir, process)


def export_table(table: pd.DataFrame, path: Path, format: str, include_index: bool):
    if format == "md":
        if len(table.columns) > 6:
            split = (len(table.columns) + 1) // 2
            output = table.iloc[:, :split].astype(str).to_markdown(index=include_index, tablefmt="pretty") + "\n" + \
                     table.iloc[:, split:].astype(str).to_markdown(index=include_index, tablefmt="pretty")
        else:
            output = table.astype(str).to_markdown(index=include_index, tablefmt="pretty")
    elif format == "tex":
        output = table.to_latex(index=include_index)
    elif format == "csv":
        output = table.to_csv(index=include_index)
    elif format == "json":
        output = table.to_json(index=include_index)
    else:
        raise NotImplementedError(f"Table format {format} not supported")

    with open(path, "w") as file:
        file.write(output)


def visualize_metrics(args):
    results_dir = args.results_dir
    format = args.format
    dpi = args.dpi
    def process(output_dir_path: Path, metrics: dict, *_):
        plots = create_plots(metrics)
        for name, fig in plots:
            fig.savefig(output_dir_path / f"{name}.{format}", format=format, dpi=dpi)
            plt.close(fig)
    process_metrics(results_dir, process)


def create_setups_summary(args):
    results_dir = args.results_dir
    results_dir_path = Path(results_dir).absolute()
    format = args.format

    data = dict()
    def process(_, metrics: dict, setup: str, formula: str):
        extract_setup_summary_data(metrics, data, setup, formula)
    process_metrics(results_dir, process)
    table = create_setup_summary_table(data, experiment_setups)
    export_table(table, results_dir_path / f"summary_data.{format}", format, True)


def visualize_setup_summaries(args):
    data = args.data
    data_path = Path(data).absolute()
    output_dir = args.output_dir
    output_dir_path = Path(output_dir).absolute()
    format = args.format
    dpi = args.dpi

    df = pd.read_csv(data_path, header=[0, 1], index_col=0)
    plots = create_setup_summary_plots(df, experiment_setups)
    for name, fig in plots:
        fig.savefig(output_dir_path / f"{name}.{format}", format=format, dpi=dpi)
        plt.close(fig)


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
parser_run.set_defaults(func=run_experiments)
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
parser_summary.add_argument("results-dir",
                            type=str,
                            help="Directory with results (given as '--results-dir' when running experiments)")
parser_summary.add_argument("-f", "--format", type=str, default="md", help="Format of exported tables")

parser_visualize = subparsers.add_parser("visualize",
                                         description="Process metrics from experiments and visualize them",
                                         formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_visualize.set_defaults(func=visualize_metrics)
parser_visualize.add_argument("results-dir",
                              type=str,
                              help="Directory with results (given as '--results-dir' when running experiments)")
parser_visualize.add_argument("-f", "--format", type=str, default="png", help="Format of plot files")
parser_visualize.add_argument("-r", "--dpi", "--resolution", type=int, default=150, help="Resolution of plots")

parser_setup_summary = subparsers.add_parser("setup-summary",
                                             description="Process metrics from experiments and create summary table",
                                             formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_setup_summary.set_defaults(func=create_setups_summary)
parser_setup_summary.add_argument("results-dir",
                              type=str,
                              help="Directory with results (given as '--results-dir' when running experiments)")
parser_setup_summary.add_argument("-f", "--format", type=str, default="csv", help="Format of the table")

parser_visualize_setup_summary = subparsers.add_parser("visualize-setup-summary",
                                                       description="Process metrics from experiments and generate plots"
                                                       " comparing experiment setups",
                                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_visualize_setup_summary.set_defaults(func=visualize_setup_summaries)
parser_visualize_setup_summary.add_argument("data",
                                            type=str,
                                            help="File containing summary data as a CSV")
parser_visualize_setup_summary.add_argument("-o", "--output-dir", type=str, default=".",
                                            help="Directory to save the plots to")
parser_visualize_setup_summary.add_argument("-f", "--format", type=str, default="png", help="Format of plot files")
parser_visualize_setup_summary.add_argument("-r", "--dpi", "--resolution", type=int, default=150,
                                            help="Resolution of plots")

if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
