import os
import sys
import subprocess
from pathlib import Path
from typing import Generator

script_root_path: Path = Path(os.path.realpath(__file__)).parent.absolute()
default_config_path: Path = script_root_path / "default_config.toml"
inputs_path: Path = script_root_path / "inputs"
setups_path: Path = script_root_path / "setups"
results_path: Path = script_root_path / "results"

input_formulas: list[str] = [
    "bf0432-007",
]

experiment_setups: list[str] = [
    "minimal_bloat",
]


def generate_setups() -> Generator[tuple[Path, Path, Path, Path], None, None]:
    for formula in input_formulas:
        for setup in experiment_setups:
            setup_config_path = setups_path / f"{setup}.toml"
            input_config_path = inputs_path / f"{formula}.toml"
            input_formula_path = inputs_path / f"{formula}.cnf"
            output_dir_path = results_path / f"{setup}__{formula}"
            yield setup_config_path, input_config_path, input_formula_path, output_dir_path


def run_dp_experiments(dp_path: Path):
    for setup_config_path, input_config_path, input_formula_path, output_dir_path in generate_setups():
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

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path_to_dp_executable>")
        sys.exit(1)
    dp_path = Path(sys.argv[1])
    if not dp_path.exists():
        print(f"Invalid path to dp executable: {dp_path}")
        sys.exit(1)
    run_dp_experiments(dp_path)
