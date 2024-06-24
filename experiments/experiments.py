#!/usr/bin/env python3

import os
import sys
import subprocess
import time
import argparse
import json
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import timedelta
from typing import Callable, Generator
from tables import create_tables
from plots import create_plots
from summary_plots import extract_setup_summary_data, create_setup_summary_plots

# path constants
script_root_dir: Path = Path(os.path.realpath(__file__)).parent.absolute()
input_formulas_list_path: Path = script_root_dir / "old_input_formulas.txt"
default_config_path: Path = script_root_dir / "default_config.toml"
inputs_dir: Path = script_root_dir / "inputs"
setups_dir: Path = script_root_dir / "setups"

# experiments
input_formulas: list[str] = None
experiment_setups: list[str] = [
    "default",
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
    for i, (_, _,
            setup_config_path,
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
    if num_processes > 1:
        print()
        for dir, code in zip(working_dirs, exit_codes):
            print(f"Experiment {dir.name} exited with code {code}")


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


def visualize_setup_summaries(args):
    results_dir = args.results_dir
    format = args.format
    dpi = args.dpi

    data = dict()
    def process(_, metrics: dict, setup: str, formula: str):
        extract_setup_summary_data(metrics, data, setup, formula)
    process_metrics(results_dir, process)
    plots = create_setup_summary_plots(data)
    for name, fig in plots:
        fig.savefig(results_dir / f"{name}.{format}", format=format, dpi=dpi)
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

parser_setup_summary = subparsers.add_parser("setup-summary",
                                             description="Process metrics from experiments and generate plots comparing"
                                                         " experiment setups",
                                             formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_setup_summary.set_defaults(func=visualize_setup_summaries)
parser_setup_summary.add_argument("results_dir",
                              type=str,
                              help="Directory with results (given as '--results-dir' when running experiments)")
parser_setup_summary.add_argument("-f", "--format", type=str, default="png", help="Format of plot files")
parser_setup_summary.add_argument("-r", "--dpi", "--resolution", type=int, default=150, help="Resolution of plots")

if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
