#!/usr/bin/env python3

import sys
import os
import itertools
import json
import argparse
import numpy as np
import pandas as pd
from typing import Generator
from pathlib import Path
from experiments import setups_dir, inputs_dir, default_config_path
from run import run_experiment
from summary_plots import get_duration, get_remaining_vars_ratio


def get_distribution(start: float, stop: float, num: int, shift: float = 0.2):
    return np.geomspace(start + shift, stop + shift, num=num) - shift

input_formulas: list[str] = [
    "instancesCompilation/Handmade/LatinSquare/qg1-07.dc.min",
    "instancesCompilation/Handmade/ais/ais6.dc.min",
    "instancesCompilation/Handmade/parity/par16-3-c.dc.min",
    "instancesCompilation/random/uf250-1065/uf250-036.dc.min",
]
experiment_setups: list[str] = [
    "grid_search",
]
grid: dict[str, np.ndarray] = {
    "complete-minimization-relative-size": get_distribution(1, 3, 5, -0.7).round(2),
    "partial-minimization-relative-size": get_distribution(0, 2, 5, 0.1).round(2),
    "incremental-absorption-removal-relative-size": get_distribution(0, 2, 5, 0.1).round(2),
}


def generate_configs() -> Generator[tuple[str, np.float64], None, None]:
    value_combinations = itertools.product(*grid.values())
    for values in value_combinations:
        yield [(o, v) for o, v in zip(grid.keys(), values)]


def generate_setups(results_dir: Path) -> Generator[tuple[tuple[str, np.float64], str, Path, Path, Path, Path, list[str]], None, None]:
    for formula in input_formulas:
        for setup in experiment_setups:
            for config in generate_configs():
                setup_config_path = setups_dir / f"{setup}.toml"
                input_config_path = inputs_dir / f"{formula}.toml"
                input_formula_path = inputs_dir / f"{formula}.cnf"
                config_name = "_".join([f"{o[:3]}={v:.2f}" for o, v in config])
                output_dir_path = results_dir / f"{setup}/{formula}/{config_name}"
                extra_options = [(f"--{o}", f"{v:.2f}") for o, v in config]
                extra_options = [item for pair in extra_options for item in pair]
                yield config, formula, setup_config_path, input_config_path, input_formula_path, output_dir_path, extra_options


def run_grid_search(dp_path: Path, results_dir: Path, setup_index: int):
    if not dp_path.exists():
        print(f"Invalid path to dp executable: {dp_path}", file=sys.stderr)
        sys.exit(1)
    # prepare execution setups
    commands: list[list[str]] = []
    working_dirs: list[Path] = []
    for i, (_, _,
            setup_config_path,
            input_config_path,
            input_formula_path,
            output_dir_path,
            extra_options) in enumerate(generate_setups(results_dir)):
        if setup_index >= 0 and i != setup_index:
            continue
        command_with_args = [
            str(dp_path.absolute()),
            str(input_formula_path),
            "--config", str(default_config_path),
            "--config", str(setup_config_path),
        ]
        if input_config_path.exists():
            command_with_args += ["--config", str(input_config_path)]
        command_with_args += extra_options
        os.makedirs(output_dir_path, exist_ok=True)
        commands.append(command_with_args)
        working_dirs.append(output_dir_path)
    # execute
    in_parallel = [False for _ in range(len(commands))]
    iter = map(run_experiment, commands, working_dirs, in_parallel)
    for _ in iter:
        pass


def extract_results(results_dir: Path):
    index = pd.MultiIndex.from_product([input_formulas] + list(grid.values()), names=["formula"] + list(grid.keys()))
    df = pd.DataFrame(index=index, columns=["duration", "remaining_vars"])
    for config, formula, _, _, _, output_dir_path, _ in generate_setups(results_dir):
        metrics_path = output_dir_path / "metrics.json"
        if metrics_path.exists() and metrics_path.is_file():
            config_vals = (v for _, v in config)
            with open(metrics_path, "r") as file:
                metrics: dict = json.load(file)
                duration = get_duration(metrics)
                remaining_vars = get_remaining_vars_ratio(metrics)
                df.loc[(formula, *config_vals), "duration"] = duration
                df.loc[(formula, *config_vals), "remaining_vars"] = remaining_vars
        else:
            print(f"Metrics file {metrics_path} doesn't exist", file=sys.stderr, flush=True)
    df.to_csv(results_dir / "results.csv")


def count_setups(args):
    print(len(list(generate_setups(Path()))))


def run_search(args):
    run_grid_search(Path(args.dp_executable), Path(args.results_dir), args.setup_index)


def process_results(args):
    extract_results(Path(args.results_dir))

# CLI
parser = argparse.ArgumentParser(description="Run grid search for DP hyperparameters",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
subparsers = parser.add_subparsers()

parser_count = subparsers.add_parser("count",
                                     description="Count the number of experiment setups that will be run",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_count.set_defaults(func=count_setups)

parser_run = subparsers.add_parser("run",
                                   description="Run grid search",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_run.set_defaults(func=run_search)
parser_run.add_argument("dp_executable", type=str, help="Path to the compiled DP executable")
parser_run.add_argument("-r", "--results-dir", type=str, default="results_grid_search", help="Directory for storing results")
parser_run.add_argument("-i", "--setup-index",
                        type=int,
                        default=-1,
                        help="Run only one experimental setup at the given index")

parser_process = subparsers.add_parser("process",
                                       description="Process results into a single table",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser_process.set_defaults(func=process_results)
parser_process.add_argument("results_dir",
                            type=str,
                            help="Directory with results (given as '--results-dir' when running grid search)")

if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
