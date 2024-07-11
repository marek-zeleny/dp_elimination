import os
import sys
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor
from datetime import timedelta
from pathlib import Path


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


def run_dp_experiments(dp_path: Path, results_dir: Path, num_processes: int, setup_index: int):
    from experiments import generate_setups, default_config_path

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
            output_dir_path) in enumerate(generate_setups(results_dir.absolute())):
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
